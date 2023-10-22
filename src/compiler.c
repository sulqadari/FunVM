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
 * Each local has a name represented by the struct Token and
 * the nesting level, where it appears, e.g. 'zero' is
 * the global scope, '1' is the first top-level
 * block, two is inside the previous one, and so on.
 * 
 * Token name:		Name of the local variable;
 * 
 * FN_WORD depth:	The scope depth of the block where the
 * 					local variable is declared. This filed is
 * 					initialized with the current value of
 * 					Compiler::scopeDepth every time 'addLocal()'
 * 					is called.
 */
typedef struct {
	Token name;
	FN_WORD depth; 
} LocalVariable;

typedef struct {
	FN_UBYTE index;
	bool isLocal;
} Upvalue;

/*
 * The distinction this enum provides is meaningful in two places:
 * - 
 * - 
 */
typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT
} FunctionType;

/**
 * Compiler tracks the scopes and variables within them
 * at compile time, taking most of the advantages from 
 * stack-based local variable declaration pattern.
 * 
 * The Compiler stores data like: which slots are owned by which local
 * variables, how many block of nesting we're currently in, etc.
 * Because all these metadata is specific to a single function, when
 * we need to create a new one, the 'function()' function creates a new
 * separate compiler on the 'C' stack for each function being compiled.
 * 
 * The compiler simulates the stack during compilation to note
 * on which stack offset variable lives. These offsets
 * are used as operands for the bytecode instructions that read
 * and read store local variables.
 * 
 * NOTE: Since the instruction operand being used to encode
 * a local is a single byte, the current VM implementation has
 * a hard limit on the number of locals that can be in scope at once.
 *  
 * The Complier struct contains the following fields:
 * LocalVariable locals[]:	array of local variables of fixed size (see NOTE)
 *					ordered in declaration appearance sequence.

 * localCount:		tracks the number of locals are in the scope

 * scopeDepth:		the number of blocks surrounding the current
 *					bit of code we're compiling;
 */
typedef struct Compiler {

	/* Each compiler points back to the Compiler for the function
	 * that encloses it, all the way back to the root Compiler
	 * for the top-level code. */
	struct Compiler* enclosing;

	/* A reference to the function object being built. */
	ObjFunction* function;

	/* Used to inform whether compiler processes top-level code
	 * (i.e. Global script) or just another function. */
	FunctionType type;

	/* Keeps track of which stack slots are associated with which
	 * local variables or temporaries. */
	LocalVariable locals[UINT8_COUNT];
	FN_WORD localCount;

	Upvalue upvalues[UINT8_COUNT];
	FN_WORD scopeDepth;
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
emitByte(FN_UBYTE byte)
{
	writeBytecode(currentContext(), byte, parser.previous.line);
}

/**
 * Writes an opcode followed by a one-byte operand.
 */
static void
emitBytes(FN_UBYTE byte1, FN_UBYTE byte2)
{
	emitByte(byte1);
	emitByte(byte2);
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

	/* Garbage collection-related paranoia. */
	compiler->function = NULL;
	compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;

	/* Allocate a new function object to compile into.
	 * NOTE: functions are created during compilation and
	 * simply invoked at runtime. */
	compiler->function = newFunction();
	currCplr = compiler;

	/* Grab the name of scoped function or a method. */
	if (TYPE_SCRIPT != type) {
		currCplr->function->name = copyString(parser.previous.start,
											parser.previous.length);
	}

	/* At the Compiler::locals[0] the VM stores the function being called.
	 * 
	 * The entry at this slot will have no name, thus can't be referenced
	 * from the user space. */
	LocalVariable* local = &currCplr->locals[currCplr->localCount++];
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

	/* Grab the function created by the current compiler. */
	ObjFunction* function = currCplr->function;

#ifdef FUNVM_DEBUG
	if (!parser.hadError) {

		/* Notice the check in here to see if the function's name is NULL?
		 * User-defined functions have name, whilst implicit function
		 * we create for the top-level code doesn't. */
		disassembleBytecode(currentContext(), (function->name != NULL) ?
										function->name->chars : "<script>");
	}
#endif // !FUNVM_DEBUG

	/* When a Compiler finishes, it pops itself off the stack by
	 * restoring previous compiler to be the new current one. */
	currCplr = currCplr->enclosing;

	return function;
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
	while ((0 < currCplr->localCount) &&
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
 * NOTE: the equality of number of parameters and the number of arguments is checked
 * at runtime 
 */
static FN_UBYTE
argumentList(void)
{
	FN_UBYTE argCount = 0;
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if (MAX_ARITY <= argCount) {
				error("Can't have more than 16 arguments");
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
	FN_UBYTE argCount = argumentList();
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
 * @returns FN_UBYTE - an offset within Constant pool where the value is stored.
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
		printf("offset = %d\n", offset);
		printf("constant pool count: %d\n",
				currCplr->function->bytecode.constPool.count);
		printf("constant pool capacity: %d\n",
				currCplr->function->bytecode.constPool.capacity);
		error("Exceed the maximum size of Constant pool.");
		return (0);
	}

	return offset;
}

static void
emitConstant(Value value)
{
	FN_UWORD offset = makeConstant(value);
	emitBytes(OP_CONSTANT, (FN_UBYTE)offset);
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
 * 
 * Walk the array of local variables beginning from the inner most
 * scope to the outer most one and try to find a variable with the
 * name passed in as argument to this function.
 * Backward moving ensures that the inner most variable will shadow the
 * outer most one.
 * 
 * @returns FN_WORD: an index in the Compiler::locals[] array if found.
 * 					 (-1) otherwise.
 */
static FN_WORD
resolveLocal(Compiler* compiler, Token* name)
{
	for (FN_WORD i = compiler->localCount - 1; i >= 0; --i) {

		LocalVariable* local = &compiler->locals[i];

		/* Return the index if names match. */
		if (identifiersEqual(name, &local->name)) {

			/* Now consider that a user tries to do the following:
			 * { var a = a; }
			 * Thus, the (-1) is the sentinel which prevents a local variable
			 * to be initialized with its own uninitialized state.
	 		 */
			if ((-1) == local->depth)
				error("Can't read local variable in its own initializer.");

			return i;
		}
	}

	return (-1);
}

/**
 * @param Compiler* the compiler of the current function.
 * @param FN_BYTE	the index of upvalue in Upvalues array.
 * @param bool		whether the closure captures a local variable or
 * an upvalue from the surrounding function.
*/
static FN_WORD
addUpvalue(Compiler* compiler, FN_UBYTE index, bool isLocal)
{
	FN_WORD upvalueCount = compiler->function->upvalueCount;

	/* Reuse an upvalue if the function already has an upvalue that
	 * closes over a variable under the given index. */
	for (FN_WORD i = 0; i < upvalueCount; ++i) {
		Upvalue* upvalue = &compiler->upvalues[i];
		if (upvalue->index == index && upvalue->isLocal == isLocal) {
			return i;
		}
	}

	if (UINT8_COUNT == upvalueCount) {
		error("Too many closure variables in function.");
		return 0;
	}

	compiler->upvalues[upvalueCount].isLocal = isLocal;
	compiler->upvalues[upvalueCount].index = index;
	return compiler->function->upvalueCount++;
}

/**
 * Looks for a local variable declared in any of the surrounding
 * scopes and functions. If it finds one, it returns an "upvalue index"
 * for that variable.
 * This function is called after failing to resolve a local variable in the
 * current function's scope, so we know the variable isn't in the current
 * compiler.
*/
static FN_WORD
resolveUpvalue(Compiler* compiler, Token* name)
{
	/* If there is no eclosing function,
	 * the variable is treated as global. */
	if (NULL == compiler->enclosing)
		return (-1);
	
	/* Find the matching variable in the enclosing function. */
	FN_WORD local = resolveLocal(compiler->enclosing, name);
	if ((-1) != local)
		return addUpvalue(compiler, (FN_UWORD)local, true);
	
	/* Recursively look up a local variable beyond the immediately
	 * enclosing function. */
	FN_WORD upvalue = resolveUpvalue(compiler->enclosing, name);
	if ((-1) != upvalue)
		return addUpvalue(compiler, (FN_UWORD)local, false);

	return (-1);
}

/**
 * Takes the given token of a global variable and adds its lexeme to
 * the bytecode's constant pool as a string.
 * @returns FN_UWORD: an index of the constant in the constant pool.
 */
static FN_UWORD
identifierConstant(Token* name)
{
	ObjString* str = copyString(name->start, name->length);
	FN_UWORD offset = makeConstant(OBJECT_PACK(str));

	return offset;
}

/**
 * Implements an access either to local or global variable. Whichever
 * type of variable we are targeted at, this function handles both type
 * of operation - setting and getting a value.
 */
static void
namedVariable(Token name, bool canAssign)
{
	FN_UBYTE getOp, setOp;

	/* Try to find a local variable with the given name. */
	FN_WORD offset = resolveLocal(currCplr, &name);

	if ((-1) != offset) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else if ((offset = resolveUpvalue(currCplr, &name)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	} else {
		offset = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (canAssign && match(TOKEN_EQUAL)) {	// If we find an equal sign, then..
		expression();						// ..evaluate the expression and..
		emitBytes(setOp, (FN_UBYTE)offset);	// ..set the value.
	} else {								// Otherwise
		emitBytes(getOp, (FN_UBYTE)offset);	// get the variable's value.
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
	[TOKEN_IDENTIFIER]		= {variable,	NULL,   PREC_NONE},
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
 * Generates a bytecode instruction and placeholder operand
 * for conditional branching.
 * @returns the offset of the emitted OP_JUMP_IF_FALSE instruction
 * in the bytecode.
 */
static FN_UWORD
emitJump(FN_UBYTE instruction)
{
	emitByte(instruction);
	emitByte(0xFF);
	emitByte(0xFF);
	
	/* ins, off1, off2. Thus, to get the offset of the instruction
	 * we need to subtract two operands. */
	return currentContext()->count - 2;
}

/**
 * Calculates the distance to jump over and stores its length value
 * in two operands of OP_JUMP and OP_JUMP_IF_FALSE bytecodes.
 *
 * Consider example:
 * if (false) {
 * 	println("if ()");
 * } else {
 * 	println("else");
 * }
 * Which have been translated to the following bytecode:
 *
 * 0000       1    OP_FALSE        
 * 0001       |    OP_JUMP_IF_FALSE    1 -> 11
 * 0002       |    op1                 0
 * 0003       |    op2                 7	// the distance to jump over
 * 0004       |    OP_POP          
 * 0005       2    OP_CONSTANT         0
 * 0006       |    op1              if ()'
 * 0007       |    OP_PRINTLN      
 * 0008       3    OP_JUMP             8 -> 15
 * 0009       |    op1                 0
 * 0010       |    op2                 4	// the distance to jump over
 * 0011       |    OP_POP          
 * 0012       4    OP_CONSTANT         1
 * 0013       |    op1              else'
 * 0014       |    OP_PRINTLN      
 * 0015       6    OP_NIL          
 * 0016       |    OP_RETURN
 *
 * Here, at runtime the 'OP_FALSE' results in adding '07' to the
 * 'frame->ip' instruction pointer, which results in stepping over to 0011:
 *
 * frame->ip = 0000;
 * ins = *frame->ip++;						// ins == OP_FALSE; frame->ip == 0001
 * push(BOOL_PACK(false));					// push OP_FALSE onto the stack
 * ins = *frame->ip++;						// ins == OP_JUMP_IF_FALSE; frame->ip == 0002
 * offset = (frame->ip[0] | frame->ip[1])	// offset == 07
 * frame->ip += 2;							// frame->ip == 0004
 * frame->ip += offset						// frame->ip == 0011
 *
 * @param FN_UWORD offset: the offset of OP_JUMP_IF_FALSE instruction
 * to jump to.
 */
static void
patchJump(FN_UWORD offset)
{
	/* Calculate how far to jump. */
	FN_UWORD jump = currentContext()->count - offset - 2;

	if (UINT16_MAX <= jump)
		error("Too much code to jump over.");

	/** Beginning from the op1, assign the actual offset value to jump to. */
	currentContext()->code[offset]		= (FN_UBYTE)((jump >> 8) & 0xFF);
	currentContext()->code[offset + 1]	= (FN_UBYTE)(jump & 0xFF);
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
 * Consider this example and bytecode it has produced:
 * if (true && true)
 *     println("true && true");
 *
 * 0000	   1 	OP_TRUE         
 * 0001	   | 	OP_JUMP_IF_FALSE    1 -> 6
 * 0002	   | 	op1                 0
 * 0003	   | 	op2                 2
 * 0004	   | 	OP_POP          
 * 0005	   | 	OP_TRUE         
 * 0006	   | 	OP_JUMP_IF_FALSE    6 -> 16
 * 0007	   | 	op1                 0
 * 0008	   | 	op2                 7
 * 0009	   | 	OP_POP          
 * 0010	   2 	OP_CONSTANT         0
 * 0011	   | 	op1              `true && true`
 * 0012	   | 	OP_PRINTLN      
 * 0013	   | 	OP_JUMP            13 -> 17
 * 0014	   | 	op1                 0
 * 0015	   | 	op2                 1
 * 0016	   | 	OP_POP          
 * 0017
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

static void
markInitialized(void)
{
	/* In case we define a top-level function there is no local variables
	 * to mark as initialized - the function is bound to a global variable. */
	if (0 == currCplr->scopeDepth)
		return;

	currCplr->locals[currCplr->localCount - 1].depth = currCplr->scopeDepth;
}

/**
 * Defining variable is when a variable becomes available for use.
 * 
 * In case of global variable this function emits the bytecode for
 * storing value in the global variable hash table.
 * 
 * In case of local variables this function marks it as initialized
 * and returns.
 *
 * In case of function declaration it simply uses variable as a reference
 * to it.
 */
static void
defineVariable(FN_UWORD global)
{
	/* 
	 * If currCplr->scopeDepth is greater than 0, then we a in local
	 * scope.
	 * There is no code to create a local variable at runtime,
	 * 
	 * Consider the state the VM is in:
	 * The variable is declared and the value is sitting
	 * right on top of the stack as the only remaining temporary.
	 * The locals are allocated at the top of the stack - right where
	 * that value already is.
	 * 
	 * Thus, there's nothing to do: the temporary simply *becomes*
	 * the local variable. Just mark the variable as 'initialized'
	 * and return. */
	if (0 < currCplr->scopeDepth) {
		markInitialized();
		return;
	}

	emitBytes(OP_DEFINE_GLOBAL, (FN_UBYTE)global);
}

/**
 * Adds a variable to the compiler's list of LocalVariable
 * in the current scope. */
static void
addLocal(Token name)
{
	if (UINT8_COUNT <= currCplr->localCount) {
		error("Too many local variables in function.");
		return;
	}

	/* Fetch the pointer to an element under 'currCplr->localCount'
	 * and proceed to its initialization. */
	LocalVariable* local = &currCplr->locals[currCplr->localCount++];
	local->name = name;

	/* To prevent an uninitialized variable to be initialized by its own value.
	 * consider example:
	 * {
	 *     var a = "outer";
	 *     {
	 *         var a = a;
	 *     }
	 * }
	 * Splitting a variable's declaration into two phases (declaration
	 * and initialization) addresses this issue.
	 * Later, once the variable's intializer has been compiled, this
	 * sentinel will be discarded.
	 */
	local->depth = -1;
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
	 * Starting from the last (and actually current) scope, e.g.,
	 * { { { } } }
	 *	    ^__here,
	 *
	 * go backward through the array and examine variables
	 * until we encounter one which is out of the current scope.
	 */
	for (FN_WORD i = currCplr->localCount - 1; i >= 0; --i) {

		/* Get the pointer to the element. */
		LocalVariable* local = &currCplr->locals[i];
		
		/* Stop searching if variable belongs to the outer scope.
		 * The left-hand expression handles special case when
		 * a local variable is uninitialized. */
		if ((local->depth != -1) && (local->depth < currCplr->scopeDepth))
			break;	/* ..stop searching */

		/* Otherwise check, if there is already a variable present
		 * with the name we are abount to assign to new variable. */
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

	/* Declare variable */
	declareVariable();

	/* Just return from the FunVM's function we're in the
	 * middle of execution if we're in a local scope.
	 * At runtime, locals aren't looked up by name. There is no need
	 * to stuff the variable's name into the constant pool.
	 * 
	 * So if the declaration is inside a local scope, we return a
	 * dummy table index instead. */
	if (0 < currCplr->scopeDepth)
		return 0;
	
	return identifierConstant(&parser.previous);
}

/**
 * Compiles the function itself - its parameter list and block body.
 * The code being generated by this function leaves the resulting
 * function object on top of the stack, which will be assigned to
 * dedicated variable using defineVariable() inside funDeclaration(void).
 * 
 * A stack of Compilers:
 * The compiler stores data like which slots are owned by which locals,
 * how manu blocks of nesting we're currently in, etc. All of that is
 * specific to a single function.
 * To provide the front-end to handle compiling multiple functions nested
 * within each other, this function creates a separate Compiler for each
 * function being compiled.
 * 
*/
static void
function(FunctionType type)
{
	Compiler compiler;

	/* Make this compiler the current one. Note that
	 * we don't need to dynamically allocate a new Compiler struct
	 * since all of them are reside on the C stack.
	 * Starting from this point, all of the functions that emit
	 * bytecode will write to the Bytecode owned by this
	 * compiler's function.*/
	initCompiler(&compiler, type);

	/* Doesn't have corresponding endScope() because we end Compiler
	 * completely when we reach the end of the function body,
	 * there is no need to close the lingering outermost scope. */
	beginScope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

	if (!check(TOKEN_RIGHT_PAREN)) {

		/* Semantically, a parameter is simply a local variable declared in
		 * the outermost lexical scope of the function body.
		 * Unlike local variables, there's no code here to initialize the
		 * parameter's value. */
		do {
			currCplr->function->arity++;
			if (MAX_ARITY < currCplr->function->arity)
				errorAtCurrent("Can't have more than 16 params.");

			FN_UWORD constant = parseVariable("Expect parameter name.");
			defineVariable(constant);
		} while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

	/* Compile the body. All of the functions that emit bytecode
	 * write to the chunk owned by the new Compiler's function. */
	block();

	/* Fetch the current compiler's function object. */
	ObjFunction* function = endCompiler();

	/* Store the function object being produced by the current Compiler
	 * as a constant in the *surrounding* function's constant table. */
	FN_UWORD offset = makeConstant(OBJECT_PACK(function));
	emitBytes(OP_CLOSURE, (FN_UBYTE)offset);

	for (FN_WORD i = 0; i < function->upvalueCount; ++i) {
		emitByte(compiler.upvalues->isLocal ? 1 : 0);
		emitByte(compiler.upvalues[i].index);
	}
}

/**
 * Declares function.
 * 
 * Functions are first-class values i.e. applicable to be passed over as
 * a function arguments.
 * The "declaration" stands for simply creating and storing a function
 * in a newly declared variable.
 * 
 * A function has been declared at the top level will be bound to
 * a global variable.
 * In opposite side, declaring function inside a block or other function
 * leads to binding it to a local variable.
*/
static void
funDeclaration(void)
{
	/* Parse the name just like any other variable declaration. */
	FN_UWORD global = parseVariable("Expect function name");

	/* Mark function as 'initialized' as soon as we compile the name and
	 * before its body have been compiled.
	 * It's safe for a function to refer to its own name inside its body.
	 * This trick provides support for recursive calls. */
	markInitialized();

	/* Compile the body of the function. */
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
emitLoop(FN_UWORD loopStart)
{
	emitByte(OP_LOOP);

	FN_UWORD offset = currentContext()->count - loopStart + 2;
	if (UINT16_MAX < offset)
		error("Loop body too large.");
	
	emitByte((FN_UBYTE)((offset >> 8) & 0xff));
	emitByte((FN_UBYTE) (offset & 0xff));
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
	FN_WORD exitJump = -1;

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
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");

	if (check(TOKEN_RIGHT_PAREN))
		errorAtCurrent("Expect expression inside 'if' clause.");

	/* Compile the condition expression. The resulting *condition value*
	 * will be placed on top of the stack and used to determine
	 * whether to execute the 'then' branch or skip it. */
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'if' clause.");

	/* Get the index of OP_JUMP_IF_FALSE opcode in bytecode array.
	 * We'll need it later, after 'then' body being compilled and we know
	 * where exactly its blob of bytecode ends up. Then, the operands of
	 * OP_JUMP_IF_FALSE will be backpatched with the real offset value to
	 * jump to. */
	FN_UWORD thenJump = emitJump(OP_JUMP_IF_FALSE);

	/* If condition value is 'truthy':
	 * emit OP_POP instruction right before the code inside the 'then' branch 
	 * to pull out it from top of the stack.
	 * 
	 * This way we obey the rule that each statement after being executed
	 * MUST to have 'zero stack effect' - its length should be as tall as
	 * it was before then. */
	emitByte(OP_POP);

	/* Compile the 'then' body. */
	statement();

	/* Jump over the 'else' branch. This opcode ensures that
	 * in case the execution flow enters the 'then' branch,
	 * it would not fall through into 'else', but step it over.
	 * Thus, the OP_JUMP shall be compilled unconditionally. */
	FN_UWORD elseJump = emitJump(OP_JUMP);

	/* Once we have compiled 'then' body, we know how far to jump.
	 * Thus, backpatch 'then' jump before compiling the 'else' clause. */
	patchJump(thenJump);

	/* If condition value is 'falsey':
	 * emit OP_POP instruction right before the code inside the 'else' branch 
	 * to pull out it from top of the stack. */
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

/**
 * Handles the return statement.
 */
static void
returnStatement(void)
{
	if (TYPE_SCRIPT == currCplr->type) {
		error("Can't return from top-level code.");
	}
	
	/* The return value expression is optional, so the parser looks for a semicolon
     * token to tell if a value was provided. Thus, if the semicolon follows right
	 * after 'return' token, then there is no value to return. Return NILL instead. */
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

	return ((parser.hadError) ? NULL : mainFunction);
}