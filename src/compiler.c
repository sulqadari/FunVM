#include "funvmConfig.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytecode.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "value.h"

#ifdef FUNVM_DEBUG
#include "debug.h"
#endif

typedef struct {
	Token current;	/* current token */
	Token previous;	/* previous token */
	bool hadError;	/* records a errors occured during compilation. */
	bool panicMode;	/* avoids error cascades. */
} Parser;

/**
 * The enumeration of all precedence levels in order from
 * lowest to highest.
 * Using this precedence order, consider the following example:
 * say, the compiler is sitting on a chunk of code like:
 * -a.b + c;
 * if we call parsePrecedence(PREC_ASSIGNMENT), then it will parse the
 * entire expression because '+' has highest precedence than assignment.
 * If instead we call parsePrecedence(PREC_UNARY), it will compile the '-a.b'
 * and stop there. It doesn't keep going through the '+' because the addition
 * has lower precedence than unary operators.
*/
typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,	// =
	PREC_OR,			// or
	PREC_AND,			// and
	PREC_EQUALITY,		// == !=
	PREC_COMPARISON,	// < > <= >=
	PREC_TERM,			// + -
	PREC_FACTOR,		// * /
	PREC_UNARY,			// ! -
	PREC_CALL,			// . ()
  	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

/* Represents a single row in the parser table. */
typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

/** 
 * Local variable representation struct.
 * Each local has a name represented by the struct Token
 * and the nesting level where it appears, e.g. 'zero'
 * is the global scope, '1' is the first top-level
 * block, two is inside the previous one, and so on.
 */
typedef struct {
	Token name;
	FN_WORD depth; 
} Local;

typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT
} FunctionType;

/**
 * Compiler tracks the scopes and variables within them
 * at compile time, taking most of the advantages from 
 * stack-based local variable declaration pattern.
 * 
 * The Complier struct contains the following fields:
 * Local locals[]:	all locals that are in scope during each
 * 				point in the compilation process;
 *				Ordered in declaration appearance sequence.
 * localCount:		the number of locals are in the scope, i.e.,
 *				how many of those array slots are in use;
 * scopeDepth:		the number of blocks surrounding the current
 *				bit of code we're compiling;
 */
typedef struct Compiler {

	/* Each compiler points back to the Compiler for the function
	 * that encloses it, all the way back to the root Compiler
	 * for the top-level code. */
	struct Compiler* enclosing;

	/* Instead of pointing directly to a Bytecode that the
	 * compiler writes to, it instead has a reference to the
	 * function object being built. */
	ObjFunction* function;

	/* Distinguishes compilation between top-level code and
	 * the body of a function.
	 * Note that most of the compiler's logic doesn't care about
	 * this distinction, except few points. */
	FunctionType type;

	/* Keeps track of which stack slots are associated with which
	 * local variables or temporaries.
	 * NOTE: the compiler implicitly claims stack slot zero for
	 * the VM's own internal use
	 * (additional explanation can be found in initCompiler() body). */
	Local locals[UIN8_COUNT];
	FN_UWORD localCount;
	FN_UWORD scopeDepth;
} Compiler;

static Parser parser;
static Compiler* currCplr = NULL;

/**
 * The current context is always the bytecode owned by the function
 * we're in the middle of compiling.
 */
static Bytecode*
currentContext(void)
{
	return &currCplr->function->bytecode;
}

/**
 * The top-level error handling function.
 * 
 * When an error occurs, all available information (line, lexeme and message)
 * will be printed out. Before function returns to the caller, it will
 * cock the 'panic mode' flag. This is done to prevent compiler from outputing
 * the rest messages about the errors which are the result of the first
 * one in the series of the errors. Consider the following:
 * if (& > idx) idx + 2;
 * Right after compiler processes '(& >' expression, the remaining is nothing but
 * meaningless blob of code.
 * Thus, when the compiler encounteres subsequent error it just returns without
 * annoying user with useless information.
 * This 'silent mode' will end after the compiler reaches a synchronization
 * point.
 * Consider the following code:
 * if (& > idx)
 * 		idx + 2;
 * else if (7 => idx)
 * 		idx + 3;
 * else
 * 		idx++;
 * 
 * Right after compiler throws the error message in 'if' statement it will
 * suppress any other error messages until after reaches the ';' token.
 * This ';' is the excact synchronization point.
 * After that it proceeds to 'else if' statement and will send the error
 * message about incorrent 'greater or equal' expression syntax, but silently
 * walk through until encounter ';' token, passing over other probable errors.
 * This way the compiler will process and print all errors in the single pass.
 */
static void
errorAt(Token* token, const char* message)
{
	/* Once we've encountered an error in the source code,
	 * there is no reason to print out further messages. */
	if (true == parser.panicMode)
		return;
	
	/* If we are here, then it's mean that the compiler have encountered
	 * the first error in the code. Print all relative information. */
	parser.panicMode = true;

	/* Print the line number of error occurence. */
	fprintf(stderr, "[line %d] Error", token->line);

	/* First, process the corner cases: either the current
	 * token is of type error, or designates end of file.
	 * Neither TOKEN_ERROR, nor TOKEN_EOF have human-readable lexeme. */
	switch (token->type) {
		case TOKEN_ERROR:
			// Nothing to do.
		break;
		case TOKEN_EOF:
			// Error occured at the very end of a file.
			fprintf(stderr, " at end");
		break;
		default:
			// Print out lexeme (in case it's human-readable).
			fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

/**
 * Passes the previous token followed by error message
 * to the errorAt() function.
 * Compared to errorAtCurrent() function, this one is used more
 * frequently, when either consume() or advance() functions
 * have returned from the call.
 */
static void
error(const char* message)
{
	errorAt(&parser.previous, message);
}

/**
 * Passes the current token followed by error message
 * to the errorAt() function.
 * This function is primarily used in a functions that
 * in the middle of token processing, e.g. advance() and consume().
 */
static void
errorAtCurrent(const char* message)
{
	errorAt(&parser.current, message);
}

/**
 * Steps forward through the token stream.
 * Calling scanToken() it asks the scanner for the
 * next token and stores it for later use.
 * The 'parser.previous' field is initialized with the
 * current token so that we can track the beginning
 * of the lexeme's name.
 *
 * NOTE: the scanToken() doesn't report lexical errors, instead
 * it creates special error token and leaves it up to the scanner
 * to report them.
 */
static void
advance(void)
{
	/* Stash the old 'current' token. */
	parser.previous = parser.current;

	while (true) {
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR)
			break;
		/* Passing in a message about the error type. */
		errorAtCurrent(parser.current.start);
	}
}

/**
 * Validates and consumes current token.
 * Throws an exception otherwise.
 */
static void
consume(TokenType type, const char* message)
{
	/* Ensure that the current token's type matchs
	 * expected value. If this is the case,
	 * then consume it return. */
	if (type == parser.current.type) {
		advance();
		return;
	}

	/* Throw error otherwise. */
	errorAtCurrent(message);
}

static bool
check(const TokenType type)
{
	return type == parser.current.type;
}

/**
 * Consumes the current token if and only if it has the given type.
 * @returns bool: true if match, false otherwise.
*/
static bool
match(TokenType type)
{
	if (!check(type))
		return false;

	advance();
	return true;
}

/**
 * Assigns a given byte to the bytecode chunk which
 * can be an opcode or an operand to an instruction.
 * This function also writes the previous token's line number
 * so that runtime errors are associated with that line.
 *
 * QUESTION: why do we pass in the previous token, not current?
 * ANSWER: because a bytecode to be written is the result of
 * token being consumed, whilst the 'current' token is still
 * waiting its turn to be processed. 
 */
static void
emitByte(FN_UWORD byte)
{
	writeBytecode(currentContext(), byte, parser.previous.line);
}

/**
 * Writes an opcode followed by a one-byte operand.
 */
static void
emitBytes(FN_UWORD byte1, FN_UWORD byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void
emitLoop(FN_UWORD loopStart)
{
	emitByte(OP_LOOP);

	FN_UWORD offset = currentContext()->count - loopStart + 2;
	if (UINT16_MAX < offset)
		error("Loop body too large.");
	
	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

/**
 * Initialises the compiler. One of the important part
 * of this procedure is initializing the top-level 'main()'
 * function.
*/
static void
initCompiler(Compiler* compiler, FunctionType type)
{
	/* Cache the current Compiler as the parent. */
	compiler->enclosing = currCplr;
	/* Allocate a new function object to compile into.
	 * NOTE: functions are created during compilation and
	 * simply invoked at runtime. */
	compiler->function = newFunction();
	compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;

	currCplr = compiler;

	/* Grab the name of the function or method. */
	if (TYPE_SCRIPT != type) {
		currCplr->function->name = copyString(parser.previous.start,
											parser.previous.length);
	}
	/* Claim the zeroth stack slot for MV's internal use. Giving
	 * the empty name protects from [un]intentional referencing
	 * from a user's space. */
	Local* local = &currCplr->locals[currCplr->localCount++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
}

static void
emitReturn(void)
{
	/* The function implicitly returns NIL. */
	emitByte(OP_NIL);
	emitByte(OP_RETURN);
}

/**
 * Returns the main function which contains a chunk of bytecode
 * compiled by the compiler.
*/
static ObjFunction*
endCompiler(void)
{
	emitReturn();
	ObjFunction* mainFunction = currCplr->function;

#ifdef FUNVM_DEBUG
	if (!parser.hadError) {

		/* Notice the check in here to see if the function's name is NULL?
		 * User-defined functions have name, whilst implicit function
		 * we create for the top-level code doesn't. */
		disassembleBytecode(currentContext(),
		(mainFunction->name != NULL) ? mainFunction->name->chars : "<script>");
	}
#endif // !FUNVM_DEBUG

	/* When a Compiler finishes, it pops itself off the stack by
	 *restoring previous compiler to be the new current one. */
	currCplr = currCplr->enclosing;
	return mainFunction;
}

/**
 * Enters a new local scope.
 * Semantically the block statements create a new scope, thus,
 * before we compile the body of a block, this function call
 * enters a new local scope. To do so, we increment the current depth.
 */
static void
beginScope(void)
{
	currCplr->scopeDepth++;
}

/**
 * Returns from the local scope.
 * When we pop a scope ,we walk backward through the local array looking for
 * any variables declared at the scope depth we just left.
 */
static void
endScope(void)
{
	currCplr->scopeDepth--;

	/* Discard locals by simply decrementing the length of the array
	 * and poping values from top of the stack at runtime. */
	while ( (0 < currCplr->localCount) &&
			(currCplr->locals[currCplr->localCount - 1].depth >
			 currCplr->scopeDepth)) {

		emitByte(OP_POP);
		currCplr->localCount--;
	}
}

static void expression(void);

/* Blocks can contain declaration, and control flow statements
 * can contain other statements, which means these two functions
 * will eventually be recursive. */
static void statement(void);
static void declaration(void);

static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void and_(bool canAssign);
static void or_(bool canAssign);

/**
 * Infix expressions parser.
 * This function compiles the right-hand operands and emits the bytecode instruction
 * for the operator.
 * The fact that the left operand e.g. '= 1 +...' or '= (1 + 2 * 3) +...' gets
 * compiled first, means that at runtime that code gets executed first.
 * When it runs, the value it produces will end up on the stack, which the
 * right where the infix operator is going to need it.
 * Then this function handles the rest of the arithmetic operators, e.g.
 * '...2;' or '...(4 / 5);'.
*/
static void
binary(bool canAssign)
{
	/* The leading operand's token has already been comsumed, as well as
	 * infix operator. Thus, take it back so that we can get to know
	 * what king of arithmetic operation we're dealing with. */
	TokenType operatorType = parser.previous.type;
	
	/* Consider the expression: '2 * 3 + 4'.
	 * When we parse the right operand of the '*' expression, we need to
	 * just capture '3', and not '3 + 4', because '+' is lower precedence
	 * than '*'.
	 * Each binary operator's right-hand operand precedence is one level
	 * higher than its own. Using getRule() we lookup operator's precedence
	 * and call parsePrecedence() with one level higher than this
	 * operator's level. This hack allows us to use the single binary()
	 * function instead of declaring a separate function set for each king
	 * of operators.
	 * One higher level of precedence is used because the binary operators
	 * are left-associative. Given a series of the same operators, like:
	 * 1 + 2 + 3 + 4,
	 * we want to parse it like:
	 * (((1 + 2) + 3) + 4),
	 * Thus, when parsing the right-hand operand to the first '+', we want
	 * to consume the '2', but not the rest, so we use one level
	 * above addition's precedence.
	 * Calling parsePrecedence() with the same precedence as the current
	 * operator is essential for right-associative operators like assignment:
	 * a = b = c = d,
	 * must be parsed as:
	 * (a = (b = (c = d))),
	 * */
	ParseRule* rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType) {
		case TOKEN_BANG_EQUAL:		emitBytes(OP_EQUAL, OP_NOT);	break;
		case TOKEN_EQUAL_EQUAL:		emitByte(OP_EQUAL);				break;
		case TOKEN_GREATER:			emitByte(OP_GREATER);			break;
		case TOKEN_GREATER_EQUAL:	emitBytes(OP_LESS, OP_NOT);		break;
		case TOKEN_LESS:			emitByte(OP_LESS);				break;
		case TOKEN_LESS_EQUAL:		emitBytes(OP_GREATER, OP_NOT);	break;

		case TOKEN_PLUS:			emitByte(OP_ADD);				break;
		case TOKEN_MINUS:			emitByte(OP_SUBTRACT);			break;
		case TOKEN_STAR:			emitByte(OP_MULTIPLY);			break;
		case TOKEN_SLASH:			emitByte(OP_DIVIDE);			break;

		default: return; // Unreachable
	}
}

/**
 * Steps through arguments as long as encounters commas after each expression.
 */
static FN_UWORD
argumentList(void)
{
	FN_UWORD argCount = 0;
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if (argCount > MAX_ARITY) {
				error("Can't have more than 127 arguments");
			}
			argCount++;
		} while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return argCount;
}

static void
call(bool canAssign)
{
	FN_UWORD argCount = argumentList();
	emitBytes(OP_CALL, argCount);
}

static void
literal(bool canAssign)
{
	switch (parser.previous.type) {
		case TOKEN_FALSE:	emitByte(OP_FALSE);	break;
		case TOKEN_NIL:		emitByte(OP_NIL);	break;
		case TOKEN_TRUE:	emitByte(OP_TRUE);	break;
		default: return;	// Unreachable
	}
}

/**
 * Parentheses expression handler.
 *
 * Assumes the initial opening parenthesis has already been consumed.
 * Recursively calls back into expression() to compile
 * the expression between the parenthesis, then parse the closing one
 * at the end.
 *
 * As far as the back end is concerned, there's literally nothing to a
 * grouping expression. Its sole function is syntactic - it lets insert
 * a lower-precedence expression where a higher precedence is expected.
 * Thus, it has no runtime semantics on its own and therefore doesn't emit
 * any bytecode.
 */
static void
grouping(bool canAssign)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * Push the given value into Constant Pool.
 * @returns FN_UWORD - an offset within Constant pool where the value is stored.
 */
static FN_UWORD
makeConstant(Value value)
{
	FN_UWORD offset = addConstant(currentContext(), value);

	/* If the total number of constants in constant pool
	 * exceeds 255 elements, then instead of OP_CONSTANT which
	 * occupes two bytes [OP_CONSTANT, operand], the OP_CONSTANT_LONG
	 * comes into play, which spawns over four bytes:
	 * [OP_CONSTANT_LONG, operand1, operand2, operand3] */
	if (UINT8_MAX < offset) {
		error("Exceed the maximum size of Constant pool.");
		return (0);
	}

	return (FN_UWORD)offset;
}

static void
emitConstant(Value value)
{
	emitBytes(OP_CONSTANT, makeConstant(value));
}


/**
 * Compiles number literal. Assumes the token for the
 * number literal has already been consumed and is stored
 * in 'parser.previous' field.
 */
static void
number(bool canAssign)
{
	/* Convert the lexeme to a C FN_FLOAT. */
	FN_FLOAT value = strtod(parser.previous.start, NULL);
	
	/* Wrap it in a Value and store in the constant table. */
	emitConstant(NUMBER_PACK(value));
}

/**
 * Takes the string's characters directly from the lexeme,
 * trims surrounding FN_FLOAT quotes and then creates a string object,
 * wraps it in a 'Value' and stores in into the constant pool.
 * 
 * Also makes VM track the new object in the VM's linked list so that,
 * it can be tracked and deleted before interpreter terminates.
*/
static void
string(bool canAssign)
{
	ObjString*  str = copyString(parser.previous.start + 1,
								parser.previous.length - 2);
	
	/* Trim the leading FN_FLOAT quote and exclude
	 * from the length both leading and trailing ones. */
	emitConstant(OBJECT_PACK(str));
}


static bool
identifiersEqual(Token* a, Token* b)
{
	if (a->length != b->length)
		return false;

	return memcmp(a->start, b->start, a->length) == 0;
}

/** 
 * Searches a local variable in the current scope.
 * Returns index of the variable whom identifier matches with the
 * given token's name.
 * We walk the array backward so that we find the *last* declared
 * variable with the identifier. That ensures that inner local
 * variables correctly shadow locals with the same name in surrounding
 * scopes.
 */
static FN_WORD
resolveLocal(Compiler* compiler, Token* name)
{
	for (FN_WORD i = compiler->localCount - 1; i >= 0; --i) {
		Local* local = &compiler->locals[i];
		if (identifiersEqual(name, &local->name)) {

			if ((-1) == local->depth)
				error("Can't read local variable in its own initializer.");

			return i;
		}
	}

	return (-1);
}

/**
 * Takes the given token and adds its lexeme to the bytecode's constant pool
 * as a string.
 * @returns FN_UWORD: an index of the constant in the constant pool.
 */
static FN_UWORD
identifierConstant(Token* name)
{
	ObjString* str = copyString(name->start, name->length);
	return makeConstant(OBJECT_PACK(str));
}

/**
 * Passes the given identifier token and adds its lexeme to the bytecode's
 * constant table as a string.
 */
static void
namedVariable(Token name, bool canAssign)
{
	FN_UWORD getOp, setOp;
	FN_WORD offset = resolveLocal(currCplr, &name);

	if ((-1) != offset) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else {
		offset = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (canAssign && match(TOKEN_EQUAL)) {	// If we find an equal sign, then..
		expression();						// ..evaluate the expression and..
		emitBytes(setOp, offset);			// ..set the value.
	} else {								// Otherwise
		emitBytes(getOp, offset);			// get the variable's value.
	}
}

/**
 * Accesses a variable's value using its identifier (i.e. name).
*/
static void
variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

/**
 * Processes 'negation' and 'not' operations.
 * This function takes into account the expressions with the certain
 * precedence, so that, the '-a.b + c' expression will be parsed as follows:
 * 1. The '.' operator calls 'b' field of the 'a' structure;
 * 2. negation is performed on 'b' field;
 * 3. the rest of the expression will be parsed in the appropriate
 * part of the compiler, not being chewed through in this function, 
 * including '+ c'.
*/
static void
unary(bool canAssign)
{
	/* Grab the token type to note which unary operator we're
	 * dealing with. Note that it has already been consumed,
	 * that's why we are addressing previous token, not the current one. */
	TokenType operatorType = parser.previous.type;
	
	/* Compile the operand. Note that we write the target expression
	 * (either negation or not) *before* operand's bytecode. This is
	 * because of order of execution:
	 * 1. We evaluate the operand first which leaves its value on the stack.
	 * 2. Then we pop that value (negate or NOT'ing it) and push the result.
	 * 
	 * NOTE: passing PREC_UNARY means, that this call to parsePrecedence()
	 * will consume the only part of the expression, whom precedence is equal
	 * or greater than PREC_UNARY.
	 * */
	parsePrecedence(PREC_UNARY);

	/* Emit the operator instruction */
	switch (operatorType) {
		case TOKEN_MINUS:	emitByte(OP_NEGATE);	break;
		case TOKEN_BANG:	emitByte(OP_NOT);		break;
		default: return; // Unreachable
	}
}

/* The table of expressions parsing functions.
 * Each type of expression is processed by a specified function
 * that outputs the appropriate bytecode.
 * Given a token type, this table lets us find:
 * - the function to compile a prefix expression starting with a token of
 * that type;
 * - the function to compile an infix expression whose left operand is
 * followed by a token of that type;
 * - the precedence of an infix expression that uses that token as ann operator.
 *  */
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]		= {grouping, call, PREC_CALL},
	[TOKEN_RIGHT_PAREN]		= {NULL,     NULL,   PREC_NONE},
	[TOKEN_LEFT_BRACE]		= {NULL,     NULL,   PREC_NONE}, 
	[TOKEN_RIGHT_BRACE]		= {NULL,     NULL,   PREC_NONE},
	[TOKEN_COMMA]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_DOT]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_MINUS]			= {unary,    binary, PREC_TERM},
	[TOKEN_PLUS]			= {NULL,     binary, PREC_TERM},
	[TOKEN_SEMICOLON]		= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SLASH]			= {NULL,     binary, PREC_FACTOR},
	[TOKEN_STAR]			= {NULL,     binary, PREC_FACTOR},
	[TOKEN_BANG]			= {unary,     NULL,   PREC_NONE},
	[TOKEN_BANG_EQUAL]		= {NULL,     binary,   PREC_EQUALITY},
	[TOKEN_EQUAL]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL_EQUAL]		= {NULL,     binary,   PREC_EQUALITY},
	[TOKEN_GREATER]			= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL]	= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_LESS]			= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_LESS_EQUAL]		= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_IDENTIFIER]		= {variable,     NULL,   PREC_NONE},
	[TOKEN_STRING]			= {string,     NULL,   PREC_NONE},
	[TOKEN_NUMBER]			= {number,   NULL,   PREC_NONE},
	[TOKEN_AND]				= {NULL,     and_,   PREC_AND},
	[TOKEN_CLASS]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE]			= {literal,     NULL,   PREC_NONE},
	[TOKEN_FOR]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL]				= {literal,     NULL,   PREC_NONE},
	[TOKEN_OR]				= {NULL,     or_,   PREC_OR},
	[TOKEN_PRINT]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_PRINTLN]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_RETURN]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SUPER]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_THIS]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_TRUE]			= {literal,     NULL,   PREC_NONE},
	[TOKEN_VAR]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_WHILE]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ERROR]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EOF]				= {NULL,     NULL,   PREC_NONE},
};

/**
 * The main function which orchestrates all of the parsing functions.
 * It starts at the current token and parses any expression at the given
 * precedence level or higher.
 */
static void
parsePrecedence(Precedence precedence)
{
	/* Read the next token. */
	advance();

	/* Look up the corresponding ParseRule. 
	 * Parsing prefix expressions. The first token is always going to belong
	 * to some kind of prefix expressions, by definition. */
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (NULL == prefixRule) {
		error("Expect expression.");
		return;
	}

	/* Take into account the precedence of the surrounding expression
	 * that contains the variable.
	 *
	 * If the variable happens to be the right-hand side of an infix operator,
	 * or the operand of a unary operator, then that containing expression
	 * is too high precedence to permit the '=' sign.
	 * Thus, variable() should look for and consume the '=' only if it's
	 * in the context of a low-precedence expression.
	 *
	 * Since assignment is the lowest-precedence expression, the only time
	 * we allow an ssignment is when parsing an assigment expression or
	 * top-level expression like in an expression statement. */
	bool canAssign = (PREC_ASSIGNMENT >= precedence);

	/* Compile the prefix expression. */
	prefixRule(canAssign);

	/* Parsing infix expressions. The prefix expression we hace already compiled
	 * might be an left-hand operand for it if and only if the 'precedence'
	 * argument passed to this function is low enough to permit
	 * the infix operator or compiler hit a token that isn't an infix. */
	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && match(TOKEN_EQUAL)) {
		error("Invalid assignment target.");
	}
}

/**
 * Adds a variable to the compiler's list of variables
 * in the current scope. */
static void
addLocal(Token name)
{
	if (UIN8_COUNT <= currCplr->localCount) {
		error("Too many local variables in function.");
		return;
	}

	Local* local = &currCplr->locals[currCplr->localCount++];
	local->name = name;

	/* Special case handling, consider example:
	 * {
	 *   var a = "outer";
	 *  {
	 *    var a = a;
	 *  }
	 * }
	 * splitting a vavriable's declaration into two phases (declaration
	 * and initialization) addresses this issue.
	 **/
	local->depth = -1;
	local->depth = currCplr->scopeDepth;
}

static void
markInitialized(void)
{
	/* When top-level function declaration calls this function,
	 * there is no local variable to mark initialized - the function
	 * is bound to a global variable. */
	if (0 >= currCplr->scopeDepth)
		return;

	currCplr->locals[currCplr->localCount - 1].depth = currCplr->scopeDepth;
}

/**
 * Generates a bytecode instruction and placeholder operand
 * for conditional branching.
 * @returns the offset of the emitted instruction in the bytecode.
 */
static FN_UWORD
emitJump(FN_UWORD instruction)
{
	emitByte(instruction);
	emitByte(0xFF);
	emitByte(0xFF);
	
	/* ins, off1, off2, off3. Thus, to get the offset of the instruction
	 * we need to subtract three operands. */
	return currentContext()->count - 2;
}

/**
 * Moves backward in the bytecode to the point where OP_JUMP_IF_FALSE instruction
 * resides.
 * This function is called right before we emit the next instruction that we
 * want the jump to land on. In the case of 'if' statement, that means right after
 * we compile the 'then' branch and before we compile the next statement.
 * 
 * @param FN_UWORD offset: the offset of OP_JUMP_IF_FALSE instruction
 * to revert to.
 */
static void
patchJump(FN_UWORD offset)
{	
	/* Calculate how far to jump.
	 * '-3' is to adjust the bytecode for the jump offset itself. */
	FN_UWORD jump = currentContext()->count - offset - 2;

	if (UINT16_MAX < jump)
		error("Too much code to jump over.");
	
	/** Beginning from the op1, assign the actual offset value to jump to. */
	currentContext()->code[offset] = (FN_UWORD)((jump >> 8) & 0xFF);
	currentContext()->code[offset + 1] = (FN_UWORD)(jump & 0xFF);
}

/**
 * Compiles logical '&&' operation.
 * At the point this function is called, the left-hand side expression has
 * already been compiled, which means that at runtime its value will be on top
 * of the stack.
 * If that value is falsey, the entire '&&' must be false, so skip
 * the right operand and leave the left-hand side value as the result of
 * the entire expression.
 * Otherwise, we discard the left-hand value and evaluate the right operand
 * which becomes the result of the whole '&&' expression.
 * 
 */
static void
and_(bool canAssign)
{
	FN_UWORD endJump = emitJump(OP_JUMP_IF_FALSE);
	
	emitByte(OP_POP);
	parsePrecedence(PREC_AND);

	patchJump(endJump);
}

/**
 * Compiles logical '||' expression.
 * If the left-hand side is truthy, then we skip over the right operand
 * so that the execution flow proceeds to code.
 * Otherwise, it does a tiny jump over the next statement which is
 * unconditional jump over the code for the right operand.
*/
static void
or_(bool canAssign)
{
	FN_UWORD elseJump = emitJump(OP_JUMP_IF_FALSE);
	FN_UWORD endJump = emitJump(OP_JUMP);

	patchJump(elseJump);
	emitByte(OP_POP);

	parsePrecedence(PREC_OR);
	patchJump(endJump);
}

/**
 * Returns the rule at the given index.
*/
static ParseRule*
getRule(TokenType type)
{
	return &rules[type];
}

/**
 * Simply parses the lowest precedence level, which subsumes all of the
 * higher-precedence expressions too.
*/
static void
expression(void)
{
	parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * Keeps parsing declarations and statements until hits the closing brace.
 */
static void
block(void)
{
	while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/**
 * Defining variable is when a variable becomes available for use.
 * 
 * In case of global variable this function emits the bytecode for
 * storing value in the global variable hash table.
 * 
 * In case of local variables this function marks it as initialized
 * and returns.
 */
static void
defineVariable(FN_UWORD global)
{
	/* 
	 * is currCplr->scopeDepth is greater than 0, then we a in local
	 * scope.
	 * There is no code to create a local variable at runtime,
	 * Consider the state the VM is in:
	 * The variable is already initialized and the value is sitting
	 * right on top of the stack as the only remaining temporary.
	 * The locals are allocated at the top of the stack - right where
	 * that value already is.
	 * 
	 * Thus, there's nothing to do: the temporary simply *becomes*
	 * the local variable. */
	if (0 < currCplr->scopeDepth) {
		markInitialized();
		return;
	}

	emitBytes(OP_DEFINE_GLOBAL, global);
}

/**
 * Declaring variable is when the variable is added to scope but
 * still haven't been initialized.
 * 
 * The point where the compiler records the existence of
 * the local variable.
 * Because global variables are late bound, the compiler doesn't
 * keep track of which declarations for them it has seen.
 * But for local variables, the compiler does need to remember that
 * the variables exists.
 * Thus, declaring a local variables - is adding it to the compiler's
 * list of variables in the current scope.
 */
static void
declareVariable(void)
{
	/* Just bail out if we're in the global scope. */
	if (0 == currCplr->scopeDepth)
		return;
	
	Token* name = &parser.previous;

	/* Check if a variable with the same name have been declared
	 * previously.
	 * The current scope is always at the end of the locals[]. When we
	 * declare a new variable, we start at the end and work backward,
	 * looking for an existing variable with the same name.
	 * 
	 * NOTE: FunVM's semantic allows to have two or more variable
	 * with the same name in *different* scopes. */
	for (int i = currCplr->localCount - 1; i >= 0; --i) {

		Local* local = &currCplr->locals[i];
		if ((local->depth != -1) &&
			(local->depth < currCplr->scopeDepth)) {
			break;
		}

		if (identifiersEqual(name, &local->name))
			error("A variable with the same name is already defined in this scope.");
	}

	addLocal(*name);
}

/**
 * Consumes the identifier token for the variable name, adds its lexeme
 * to the bytecode's constant pool as a string, and then returns the
 * constant pool index where it was added.
 */
static FN_UWORD
parseVariable(const char* errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable();

	/* Exit the function if we're in a local scope.
	 * At runtime, locals aren't looked up by name. There is no need
	 * to stuff the variable's name into the constant pool, so if
	 * the declaration is inside a local scope, we return a dummy table
	 * index instead. */
	if (0 < currCplr->scopeDepth)
		return 0;
	
	return identifierConstant(&parser.previous);
}

/**
 * Compiles the function itself - its parameter list and block body.
 * The code being generated by this function leaves the resulting
 * function object on top of hte stack.
 * 
 * This function creates a separate Compiler for each function being
 * compiled to make it possible to track data like which slots are
 * owned by which local variables, how many blocks of nesting we're
 * currently, etc.
*/
static void
function(FunctionType type)
{
	Compiler compiler;
	/* Make this compiler the current one. Note that
	 * we don't need to dynamically allocate a new Compiler struct
	 * since all of them are reside on the C stack.*/
	initCompiler(&compiler, type);

	/* Doesn't have corresponding endScope() because we end Compiler
	 * completely when we reach the end of the function body,
	 * there is no need to close the lingering outermost scope. */
	beginScope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			currCplr->function->arity++;
			if (currCplr->function->arity > MAX_ARITY) {
				errorAtCurrent("Can't have more than 127 params");
			}

			FN_UWORD constant = parseVariable("Expect parameter name.");
			defineVariable(constant);
		} while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

	/* Compile the body. All of the functions that emit bytecode
	 * write to the chunk owned by the new Compiler's function. */
	block();

	/* Store the function object being produced by the current Compiler
	 * as a constant in the *surrounding* function's constant table. */
	ObjFunction* function = endCompiler();
	emitBytes(OP_CONSTANT, makeConstant(OBJECT_PACK(function)));
}

/**
 * Functions are first-class values, and a function declaration simply
 * creates and stores one in a newly declared variable. Thus, this function
 * parses the name just like any other variable declaration.
 * A function declaration at the top level will bind the function to a global
 * variable, and inside a block or other function, a function declaration
 * creates a local variable.
*/
static void
funDeclaration(void)
{
	FN_UWORD global = parseVariable("Expect function name");

	/* Marking function as 'initialized' as soon as we compile the name
	 * make it possible to support recursive local functions. */
	markInitialized();
	function(TYPE_FUNCTION);

	/* Get the function object from top of the stack and store it
	 * back into the variable we declared for it. */
	defineVariable(global);
}

/**
 * Parses variable declaration.
 */
static void
varDeclaration(void)
{
	// Manage variable name (which follows right after the 'var' keyword)
	FN_UWORD global = parseVariable("Expect variable name.");

	/* Parse the initialization. */
	if (match(TOKEN_EQUAL)) {	// is variable assigned an initialization expression?
		expression();
	} else { 						// Initialize with NIL value otherwise.
		emitByte(OP_NIL);
	}

	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

	defineVariable(global);
}

/**
 * An expression followed by a semicolon.
*/
static void
expressionStatement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_POP);
}

static void
forStatement(void)
{
	/* Wrap the whole statement in a scope, so that any declared variable
	 * will reside within it. */
	beginScope();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	/* The initializer clause.
	 * Parse token the next of opening parenthsis. */
	if (match(TOKEN_SEMICOLON)) {
		// No initializer
	} else if (match(TOKEN_VAR)) {
		varDeclaration();
	} else {

		/* Calling this function instead of expression() is because
		 * it looks for semantic-mandaroty semicolon and also emits an OP_POP. */
		expressionStatement();
	}

	/* The condition clause. */
	FN_UWORD loopStart = currentContext()->count;
	FN_UWORD exitJump = -1;

	/* Since this clause is optionalm we need to see if it's actually present.
	 * The previous semicolon was consumed by previous clause, no that if
	 * the current clause is ommited, the next token must be a semicolon,
	 * so we look for that to tell. If there is not a semicolon, there
	 * must be a condition expression. */
	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop's condition clause.");

		/* Jump out of the loop if the condition is false. */
		exitJump = emitJump(OP_JUMP_IF_FALSE);

		/* Since the jump leaves the value on the stack, we pop it before
		 * executing the body. That ensures we discard the value when
		 * the condition is true. */
		emitByte(OP_POP);
	}

	/* The increment clause.
	 * Even though this clause appears textually *before* the body, but
	 * executes *after* it.
	 * To do so, the control flow jumps over the increment clause,
	 * runs the body, jumps back up to the increment clause, runs it,
	 * and then goes to the next iteration. */
	if (!match(TOKEN_RIGHT_PAREN)) {

		/* Emit the unconditional jump that hops over the increment clause. */
		FN_UWORD bodyJump = emitJump(OP_JUMP);
		
		/* take the offset of the increment clause within bytecode. */
		FN_UWORD incrementStart = currentContext()->count;
		/* Compile the increment expression itself. The only thing we need
		 * is its side effect, so.. */
		expression();
		/* ..also emit a OP_POP to discard its value after that. */
		emitByte(OP_POP);

		consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'for' clause");

		/* Emit the main loop that takes us back to the top of the for loop,
		 * right before the condition expression if there is one. */
		emitLoop(loopStart);

		/* Change loopStart to point to the offset where the increment
		 * expression begins. When we emit the loop instruction after
		 * the body statement, this will cause it to jump up to the
		 * *increment* expression instead of teh top of the loop like
		 * it does when there is no increment. */
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	/* Compile the body. */
	statement();

	/* Emit a OP_LOOP opcode to jump back to 'condition' clause. */
	emitLoop(loopStart);

	/* Patch the exitJump if condition clause is defined. Otherwise
	 * there's no jump to patch and no condition value on the stack to pop. */
	if (-1 != exitJump) {
		patchJump(exitJump);
		emitByte(OP_POP);
	}

	endScope();
}

static void
whileStatement(void)
{
	FN_UWORD loopStart = currentContext()->count;

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'while' clause.");

	/* To skip over the subsequent body statement if the condition if falsey. */
	FN_UWORD exitJump = emitJump(OP_JUMP_IF_FALSE);
	
	/* Pop the condition value from the stack if expression is falsey. */
	emitByte(OP_POP);

	/* Compile the body. */
	statement();
	/* Emit a 'loop' instruction. */
	emitLoop(loopStart);
	patchJump(exitJump);

	/* Pop the condition value from the stack when exiting from the body
	 * (i.e. the expresssion condition was truthy. */
	emitByte(OP_POP);
}

/**
 * Compiles the if () statement.
 * The execution flow shall branch to 'then' body in case the condition
 * of 'if()' is truthy. Otherwise we step over either to the 'else' branch (if any)
 * or to the nearest instruction below the 'then' branch.
 * Every 'if()' statement had an implicit 'else' branch, even if the user didn't write
 * an 'else' clause. In the case where they left if off, all the branch does is
 * discard the condition value.
 */
static void
ifStatement(void)
{
	/* Compile the condition expresiion. The resulting value will be placed
	 * on top of the stack and used to determine whether to execute the
	 * 'then' branch or skip it. */
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'if' clause.");

	/* Get the offset of the emitted OP_JUMP_IF_FALSE instruction. */
	FN_UWORD thenJump = emitJump(OP_JUMP_IF_FALSE);

	/* If condition is truthy, then emit OP_POP to pull out condition value
	 * from top of the stack right before evaluating the code in 'then' branch.
	 * This way we obey the rule that each statement after being executed MUST
	 * to have zero stack effect - its length should be as tall as it was before. */
	emitByte(OP_POP);

	/* Compile the 'then' body. */
	statement();

	/* Note that this branch is unconditional. */
	FN_UWORD elseJump = emitJump(OP_JUMP);

	/* Once we have compiled 'then' body, we know how far to jump.
	 * Thus, proceed to 'backpatching' offset with the real value. */
	patchJump(thenJump);

	/* If condition expression have been evaluated to 'falsey' and
	 * execution flow jumped over to 'else' branch, the previous instruction
	 * OP_POP left unexecuted. Thus, fix this case. */
	emitByte(OP_POP);

	if (match(TOKEN_ELSE))
		statement();
	
	/* backpatching the 'else' branch. */
	patchJump(elseJump);
}

static void
printStatement(void)
{
	/* Evaluate the expression, leaving the result on top of the stack,
	 * which will be printed out. */
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

static void
printLnStatement(void)
{
	/* Evaluate the expression, leaving the result on top of the stack,
	 * which will be printed out. */
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINTLN);
}

static void
returnStatement(void)
{
	if (TYPE_SCRIPT == currCplr->type) {
		error("Can't return from top-level code.");
	}
	
	if (match(TOKEN_SEMICOLON)) {
		emitReturn();
	} else {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emitByte(OP_RETURN);
	}
}

/**
 * Panic mode error recovery function aimed to minimize the
 * number of cascaded compile errors that it reports.
 * The compiler exits the panic mode when it reaches a synchronization
 * point, which is statement boundaries.
*/
static void
synchronize(void)
{
	parser.panicMode = false;	/* Reset panic mode. */

	/* Skip tokens until reach domething that looks like a
	 * statement boundary. */
	while (TOKEN_EOF != parser.current.type) {
		if (TOKEN_SEMICOLON == parser.previous.type)
			return;
		
		switch (parser.current.type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;
			
			default:/* Do nothing. */;
		}

		advance();
	}
}

static void
statement(void)
{
	if (match(TOKEN_PRINT)) {
		printStatement();
	} else if (match(TOKEN_PRINTLN)) {
		printLnStatement();
	} else if (match(TOKEN_FOR)) {
		forStatement();
	} else if (match(TOKEN_IF)) {
		ifStatement();
	} else if (match(TOKEN_RETURN)) {
		returnStatement();
	} else if (match(TOKEN_WHILE)) {
		whileStatement();
	} else if(match(TOKEN_LEFT_BRACE)) {
		beginScope();
		block();
		endScope();
	} else {
		expressionStatement();
	}
}

static void
declaration(void)
{
	if (match(TOKEN_FUN)) {
		funDeclaration();
	} else if (match(TOKEN_VAR)) {
		varDeclaration();
	} else {
		statement();
	}

	/* The synchronization point. */
	if (parser.panicMode)
		synchronize();
}

/**
 * Produces the bytecode from the source file.
 * The compiler creates and returns a 'main' function that
 * contains the compiled user's program.
 * 
 * @param
 * @param
 * @returns pointer to the main function.
 */
ObjFunction*
compile(const char* source)
{
	Compiler compiler;
	
	initScanner(source);
	initCompiler(&compiler, TYPE_SCRIPT);

	parser.hadError = false;
	parser.panicMode = false;

	/* Start up the scanner. */
	advance();

	/* Keep compiling declarations until hit the end
	 * of the source file. */
	while (!match(TOKEN_EOF)) {
		declaration();
	}

	ObjFunction* mainFunction = endCompiler();

	return (parser.hadError) ? NULL : mainFunction;
}