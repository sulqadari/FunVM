#include "compiler.h"
#include "scanner.h"
#include "bytecode.h"

typedef struct {
	Token current;
	Token previous;
	bool hadError;	/* records a errors occured during compilation. */
	bool panicMode;	/* avoids error cascades. */
} Parser;

typedef enum {
	prec_none,
	prec_assignment,// =
	prec_or,		// |
	prec_and,		// &
	prec_equality,	// == !=
	prec_comparison,// < > <= >=
	prec_term,		// + -
	prec_factor,	// * /
	prec_unary,		// ! -
	prec_call,		// . ()
	prec_primary,
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence prec;
} ParseRule;

static Parser parser;
static ByteCode* currCtx;


static ByteCode*
getCurrentCtx(void)
{
	return currCtx;
}

static OpCode
getPreviousOpCode(void)
{
	uint32_t idx = getCurrentCtx()->count;
	return getCurrentCtx()->code[idx - 1];
}

static void
errorAt(Token* token, const char* message)
{
	if (true == parser.panicMode)
		return;
	
	parser.panicMode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	switch (token->type) {
		case tkn_err:
			// Nothing to do.
		break;
		case tkn_eof:
			fprintf(stderr, " at end");
		break;
		default:
			fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void
error(const char* message)
{
	errorAt(&parser.previous, message);
}

static void
errorAtCurrent(const char* message)
{
	errorAt(&parser.current, message);
}

static void
advance(void)
{
	parser.previous = parser.current;

	/* Keep looping until encounter a non-error token or reach the end of file. */
	while (true) {
		parser.current = scanToken();
		if (parser.current.type != tkn_err)
			break;
		
		errorAtCurrent(parser.current.start);
	}
}

static void
consume(TokenType type, const char* message)
{
	if (type == parser.current.type) {
		advance();
		return;
	}

	errorAtCurrent(message);
}

static bool
check(const TokenType type)
{
	return type == parser.current.type;
}

static bool
match(TokenType type)
{
	if (!check(type))
		return false;

	advance();
	return true;
}

static void
emitByte(uint8_t byte)
{
	writeByteCode(getCurrentCtx(), byte, parser.previous.line);
}

static void
emitBytes(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void
emitReturn(void)
{
	emitByte(op_ret);
}

static uint16_t
makeConstant(i32 value)
{
	int32_t idx = addConstant(getCurrentCtx(), value);
	if (idx > UINT16_MAX) {
		error("Too many constants in one chunk.");
	}

	return (uint16_t)idx;
}

static void
emitConstant(i32 value)
{
	uint16_t idx = makeConstant(value);
	if (idx > UINT8_MAX) {
		emitBytes(op_iconst_long, ((idx >> 8) & 0x00FF));
		emitByte(idx & 0x00FF);
	} else {
		emitBytes(op_iconst, idx);
	}
}

static void
commitCompilation(void)
{
	emitReturn();
}

static void expression(void);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void
binary(bool canAssign)
{
	TokenType opType = parser.previous.type;
	ParseRule* rule = getRule(opType);
	parsePrecedence((Precedence)(rule->prec + 1));

	switch (opType) {
		case tkn_neq:   emitBytes(op_eq, op_not); break;
		case tkn_2eq:   emitByte(op_eq); break;
		case tkn_gt:    emitByte(op_gt); break;
		case tkn_gteq:  emitBytes(op_lt, op_not); break;
		case tkn_lt:    emitByte(op_lt); break;
		case tkn_lteq:  emitBytes(op_gt, op_not); break;
		case tkn_plus:  emitByte(op_add); break;
		case tkn_minus: emitByte(op_sub); break;
		case tkn_star:  emitByte(op_mul); break;
		case tkn_slash: emitByte(op_div); break;
		default: return; // Unreachable
	}
}

static void
grouping(bool canAssign)
{
	expression();
	consume(tkn_rparen, "Expect ')' after expression.");
}

static void
signedInt(bool canAssign)
{
	i32 value = strtol(parser.previous.start, NULL, 10);
	emitConstant(value);
}

static void
literal(bool canAssign)
{
	switch (parser.previous.type) {
		case tkn_false: emitByte(op_false); break;
		case tkn_true:  emitByte(op_true); break;
		case tkn_null:  emitByte(op_null); break;
		default: return;
	}
}

static void
unary(bool canAssign)
{
	TokenType opType = parser.previous.type;
	
	parsePrecedence(prec_unary);
	OpCode operand = getPreviousOpCode();

	switch (opType) {
		case tkn_not:   emitByte(op_not); break;
		case tkn_minus: {
			if ((operand != op_iconst) && (operand != op_iconst_long)) {
				error("Negation is only applicable to integers.");
			}
			emitByte(op_negate);
		} break;
		default: return;
	}
}

ParseRule rules[] = {
	[tkn_lparen]   = {grouping, NULL, prec_none},
	[tkn_rparen]   = {NULL, NULL, prec_none},
	[tkn_lbrace]   = {NULL, NULL, prec_none},
	[tkn_rbrace]   = {NULL, NULL, prec_none},
	[tkn_lbracket] = {NULL, NULL, prec_none},
	[tkn_rbracket] = {NULL, NULL, prec_none},
	[tkn_semicolon] = {NULL, NULL, prec_none},
	[tkn_comma]    = {NULL, NULL, prec_none},
	[tkn_dot]      = {NULL, NULL, prec_none},
	[tkn_minus]    = {unary, binary, prec_term},
	[tkn_plus]     = {NULL,  binary, prec_term},
	[tkn_slash]    = {NULL, binary, prec_factor},
	[tkn_star]     = {NULL, binary, prec_factor},
	
	[tkn_not]      = {unary, NULL, prec_none},
	[tkn_neq]      = {NULL, binary, prec_equality},
	[tkn_eq]       = {NULL, NULL, prec_none},
	[tkn_2eq]      = {NULL, binary, prec_equality},
	[tkn_gt]       = {NULL, binary, prec_comparison},
	[tkn_gteq]     = {NULL, binary, prec_comparison},
	[tkn_lt]       = {NULL, binary, prec_comparison},
	[tkn_lteq]     = {NULL, binary, prec_comparison},
	[tkn_and]      = {NULL, NULL, prec_none},
	[tkn_or]       = {NULL, NULL, prec_none},
	
	[tkn_id]       = {NULL, NULL, prec_none},
	[tkn_str]      = {NULL, NULL, prec_none},

	[tkn_i32]      = {signedInt, NULL, prec_none},
	[tkn_if]       = {NULL, NULL, prec_none},
	[tkn_else]     = {NULL, NULL, prec_none},
	[tkn_switch]   = {NULL, NULL, prec_none},
	[tkn_break]    = {NULL, NULL, prec_none},
	[tkn_while]    = {NULL, NULL, prec_none},
	[tkn_for]      = {NULL, NULL, prec_none},
	[tkn_continue] = {NULL, NULL, prec_none},
	[tkn_class]    = {NULL, NULL, prec_none},
	[tkn_super]    = {NULL, NULL, prec_none},
	[tkn_this]     = {NULL, NULL, prec_none},
	[tkn_fun]      = {NULL, NULL, prec_none},
	[tkn_null]     = {literal, NULL, prec_none},
	[tkn_ret]      = {NULL, NULL, prec_none},
	[tkn_false]    = {literal, NULL, prec_none},
	[tkn_true]     = {literal, NULL, prec_none},
	[tkn_err]      = {NULL, NULL, prec_none},
	[tkn_eof]      = {NULL, NULL, prec_none},
};

/** Parses an expression at the given precedence level or higher. */
static void
parsePrecedence(Precedence prec)
{
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;

	if (prefixRule == NULL) {
		error("expect expression.");
	}

	bool canAssign = (prec <= prec_assignment);
	prefixRule(canAssign);

	while (getRule(parser.current.type)->prec >= prec) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && match(tkn_eq)) {
		error("Invalid assignment target.");
	}
}

static ParseRule*
getRule(TokenType type)
{
	return &rules[type];
}

static void
expression(void)
{
	parsePrecedence(prec_assignment);
}

bool
compile(const char* source, ByteCode* bCode)
{
	initScanner(source);
	currCtx = bCode;
	parser.hadError = false;
	parser.panicMode = false;

	advance();
	expression();
	consume(tkn_eof, "Expect end of expression.");
	commitCompilation();
	return !parser.hadError;
}