#include "scanner.h"

typedef struct {
	const char* start;		/* <! beginning of the current lexeme. */
	const char* current;	/* <! current character being looked at. */
	uint32_t line;			/* <! for error reporting purposes. */
} Scanner;

Scanner scanner;

void
initScanner(const char* source)
{
	scanner.start = source;
	scanner.current = source;
	scanner.line = 0;
}

static bool
isAlpha(char c)
{
	return  (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '_';
}

static bool
isDigit(char c)
{
	return c >= '0' && c <= '9';
}

static bool
isAtEnd(void)
{
	return *scanner.current == '\0';
}

/** Consumes the current character and returns it. */
static char
advance(void)
{
	scanner.current++;
	return scanner.current[-1];
}

/** Returns the current character without consuming it. */
static char
peek(void)
{
	return *scanner.current;
}

static char
peekNext(void)
{
	if (isAtEnd())
		return '\0';

	return scanner.current[1];
}

/** Conditionally consumes the next character if and only if its value
 * matches the expected one.
 */
static bool
isNext(char expected)
{
	if (isAtEnd())
		return false;
	
	if (*scanner.current != expected)
		return false;
	
	scanner.current++;
	return true;
}

static Token
makeToken(TokenType type)
{
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.length = (uint32_t)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

static Token
errorToken(const char* msg)
{
	Token token;
	token.type = tkn_err;
	token.start = msg;
	token.length = (uint32_t)strlen(msg);
	token.line = scanner.line;
	return token;
}

static void
skipWhiteSpace(void)
{
	while (true) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
			break;
			case '\n':
			scanner.line++;
			advance();
			break;
			case '/':
				if (peekNext() == '/') {
					while (peek() != '\n' && !isAtEnd()) {
						advance();
					}
				} else if(peekNext() == '*') {
					advance(); // consume '/'
					advance(); // consume '*'

					while (true) {
						if (isAtEnd())
							return;

						if (peek() == '\n')
							scanner.line++;

						if (peek() == '*' && peekNext() == '/')
							break;
						
						advance();
					}
				} else {
					return;
				}
			default: return;
		}
	}
}

static TokenType
checkKeyword(int32_t start, uint32_t length, const char* rest, TokenType type)
{
	if ((scanner.current - scanner.start) == (start + length) &&
		(memcmp(scanner.start + start, rest, length) == 0)) {
			return type;
	}

	return tkn_id;
}

static TokenType
identifierType(void)
{
	switch (scanner.start[0]) {
		
		case 'b': return checkKeyword(1, 4, "reak", tkn_break);
		case 'e': return checkKeyword(1, 3, "lse", tkn_else);
		case 'n': return checkKeyword(1, 3, "ull", tkn_null);
		case 'r': return checkKeyword(1, 5, "eturn", tkn_ret);
		case 'w': return checkKeyword(1, 4, "hile", tkn_while);
		case 'c':
			if ((scanner.current - scanner.start) > 1) {
				switch (scanner.start[1]) {
					case 'l': return checkKeyword(2, 3, "ass", tkn_class);
					case 'o': return checkKeyword(2, 6, "ntinue", tkn_continue);
				}
			}
		case 's':
			if ((scanner.current - scanner.start) > 1) {
				switch (scanner.start[1]) {
					case 'w': return checkKeyword(2, 4, "itch", tkn_switch);
					case 'u': return checkKeyword(2, 3, "per", tkn_super);
				}
			}
		case 'f':
			if ((scanner.current - scanner.start) > 1) {
				switch (scanner.start[1]) {
					case 'a': return checkKeyword(2, 3, "lse", tkn_false);
					case 'o': return checkKeyword(2, 1, "r", tkn_for);
					case 'u': return checkKeyword(2, 1, "n", tkn_fun);
				}
			}
		case 't':
			if ((scanner.current - scanner.start) > 1) {
				switch (scanner.start[1]) {
					case 'h': return checkKeyword(2, 2, "is", tkn_this);
					case 'r': return checkKeyword(2, 2, "ue", tkn_true);
				}
			}
		case 'i':
			if ((scanner.current - scanner.start) > 1) {
				switch (scanner.start[1]) {
					case 'f': return checkKeyword(0, 2, "if", tkn_if);
					case '3': return checkKeyword(0, 3, "i32", tkn_i32);
				}
			}
	}
	return tkn_id;
}

static Token
identifier(void)
{
	while (isAlpha(peek()) || isDigit(peek()))
		advance();
	
	return makeToken(identifierType());
}

static Token
number(void)
{
	while (isDigit(peek()))
		advance();
	
	return makeToken(tkn_i32);
}

static Token
string(void)
{
	while (peek() != '"' && !isAtEnd()) {
		if (peek() == '\n')
			scanner.line++;
		advance();
	}

	if (isAtEnd())
		return errorToken("Unterminated string.");
	
	// Consume the closing quote.
	advance();
	return makeToken(tkn_str);
}

Token
scanToken(void)
{
	skipWhiteSpace();
	scanner.start = scanner.current;
	if (isAtEnd())
		return makeToken(tkn_eof);
	
	char c = advance();
	if (isAlpha(c))
		return identifier();

	if (isDigit(c))
		return number();

	switch (c) {
		case '(': return makeToken(tkn_lparen);
		case ')': return makeToken(tkn_rparen);
		case '{': return makeToken(tkn_lbrace);
		case '}': return makeToken(tkn_rbrace);
		case ';': return makeToken(tkn_semicolon);
		case ',': return makeToken(tkn_comma);
		case '.': return makeToken(tkn_dot);
		case '-': return makeToken(tkn_minus);
		case '+': return makeToken(tkn_plus);
		case '/': return makeToken(tkn_slash);
		case '*': return makeToken(tkn_star);
		case '!': return makeToken(isNext('=') ? tkn_neq  : tkn_not);
		case '=': return makeToken(isNext('=') ? tkn_2eq  : tkn_eq);
		case '<': return makeToken(isNext('=') ? tkn_lteq : tkn_lt);
		case '>': return makeToken(isNext('=') ? tkn_gteq : tkn_gt);
		case '"': return string();
		// case '': return makeToken(tkn_);
		// case '': return makeToken(tkn_);
	}

	return errorToken("Unexpected character.");
}