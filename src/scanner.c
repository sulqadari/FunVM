#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
	const char* start;		/* Start of the current lexeme. */
	const char* current;	/* Current character being looked at. */
	FN_UWORD line;		/* for error reporting purposes. */
} Scanner;

Scanner scanner;

void
initScanner(const char* source)
{
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

static bool
isAlpha(char c)
{
	return	(c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c == '_');
}

static bool
isDigit(char c)
{
	return (c >= '0') && (c <= '9');
}

static bool
isAtEnd(void)
{
	return (*scanner.current == '\0');
}

/** Shifts forward the pointer to the next character in the
 * source code and returns the previous one.
*/
static char
advance(void) {
	scanner.current++;
	return scanner.current[-1];
}

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

/**
 * Compares the current character in the source code with the
 * expected value.
 * @returns bool: true if match and shifts scanner.current to the
 * subsequent character in the source file.
*/
static bool
match(char expected)
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
	token.length = (scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

static Token
errorToken(const char* message)
{
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = strlen(message);
	token.line = scanner.line;
	return token;
}

static void
skipWhiteSpace(void)
{
	for (;;) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
			break;
			case '\n':	/* Handle newline */
				scanner.line++;
				advance();
			break;
			case '/':
				if (peekNext() == '/') {

					/* A comment goes until the end of the line. 
					 * Just check the newline, but not consume it.
					 * This way, the newline will be the current
					 * character on the next turn of the outer loop. */
					while ((peek() != '\n') && !isAtEnd())
						advance();

				/* Multiple line comments. */
				} else if (peekNext() == '*') {
					advance();
					advance();
					while (!isAtEnd()) {
						if (advance() == '*') {

							if (isAtEnd())
								break;
							if (advance() == '/')
								break;
						}
					}
				} else {
					return;
				}
			break;
			default:
				return;
		}
	}
}

static TokenType
checkKeyword(int start, FN_UWORD length,
			const char* rest, TokenType type)
{
	if (((scanner.current - scanner.start) == (start + length)) &&
		memcmp(scanner.start + start, rest, length) == 0) {
			return type;
		}
	return TOKEN_IDENTIFIER;
}

/** The 'trie' algorithm. */
static TokenType
identifierType(void)
{
	switch (scanner.start[0]) {
		case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
		case 'f':
			if ((scanner.current - scanner.start) > 1) {
				switch (scanner.start[1]) {
					case 'a': return checkKeyword(2, 3,"lse", TOKEN_FALSE);
					case 'o': return checkKeyword(2, 1,"r", TOKEN_FOR);
					case 'u': return checkKeyword(2, 1,"n", TOKEN_FUN);
				}
			}
		break;
		case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
		case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
		case 'p': 
			if (scanner.current - scanner.start > 1) {
				if (checkKeyword(1, 4, "rint", TOKEN_PRINT) == TOKEN_PRINT)
					return TOKEN_PRINT;

				if (checkKeyword(1, 6, "rintln", TOKEN_PRINTLN) == TOKEN_PRINTLN)
					return TOKEN_PRINTLN;

			}
		break;
		case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
		case 't':
		if (scanner.current - scanner.start > 1) {
			switch (scanner.start[1]) {
				case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
				case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
			}
		}
		break;
		case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
		case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
		case 'N': return checkKeyword(1, 2, "um", TOKEN_NUMBER_ARRAY);
	}

	return TOKEN_IDENTIFIER;
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
	
	if ((peek() == '.') && isDigit(peekNext())) {
		/* Consume "." */
		advance();

		while (isDigit(peek()))
			advance();
	}

	return makeToken(TOKEN_NUMBER);
}

static Token
string(void)
{
	while ((peek() != '"') && !isAtEnd()) {
		if (peek() == '\n')
			scanner.line++;
		
		advance();
	}

	if (isAtEnd())
		return errorToken("Unterminated string.");
	
	/* Consume the closing quote. */
	advance();
	return makeToken(TOKEN_STRING);
}

Token
scanToken(void)
{
	skipWhiteSpace();

	scanner.start = scanner.current;
	if (isAtEnd())
		return makeToken(TOKEN_EOF);

	char c = advance();
	if (isAlpha(c)) return identifier();
	if (isDigit(c)) return number();
	switch (c) {
		case '(': return makeToken(TOKEN_LEFT_PAREN);
		case ')': return makeToken(TOKEN_RIGHT_PAREN);
		case '{': return makeToken(TOKEN_LEFT_BRACE);
		case '}': return makeToken(TOKEN_RIGHT_BRACE);
		case ';': return makeToken(TOKEN_SEMICOLON);
		case ',': return makeToken(TOKEN_COMMA);
		case '.': return makeToken(TOKEN_DOT);
		case '-': return makeToken(TOKEN_MINUS);
		case '+': return makeToken(TOKEN_PLUS);
		case '/': return makeToken(TOKEN_SLASH);
		case '*': return makeToken(TOKEN_STAR);
		case '[': return makeToken(TOKEN_LEFT_SQUARE_BRACKET);
		case ']': return makeToken(TOKEN_RIGHT_SQUARE_BRACKET);
		case '&': {
			if (match('&'))
				return makeToken(TOKEN_AND);
			else
				return errorToken("Expect the second '&' character.");
		} case '|': {
			if (match('|'))
				return makeToken(TOKEN_OR);
			else
				return errorToken("Expect the second '|' character.");
		} case '!': {
			return makeToken(
				match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		} case '=': {
			return makeToken(
				match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		} case '<': {
			return makeToken(
				match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		} case '>': {
			return makeToken(
				match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
		} case '"': return string();
	}

	return errorToken("Unexpected character.");
}