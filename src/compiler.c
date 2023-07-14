#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void
compile(const char *source)
{
	initScanner(source);
	
	/* Temporary code to drive scanner. 
	 * Scan and print one token each turn through the loop. */
	int line = -1;
	for (;;) {
		Token token = scanToken();
		if (token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		} else {
			printf("	| ");
		}
		/* The %.*s formatter consumes both 'token.length' and 'token.start' vals.
		 * The latter passes the literal represenatation of lexeme, and
		 * the former restricts the length to be printed. */
		printf("%2d '%.*s'\n", token.type, token.length, token.start);

		if (token.type == TOKEN_EOF)
			break;
	}
}