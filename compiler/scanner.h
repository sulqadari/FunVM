#ifndef FUNVM_SCANNER_H
#define FUNVM_SCANNER_H

#include "common.h"

typedef enum {
	// Single-character tokens.
	tkn_lparen,    tkn_rparen,
	tkn_lbrace,    tkn_rbrace,
	tkn_lbracket,  tkn_rbracket,
	tkn_semicolon, tkn_comma,
	tkn_dot,       tkn_minus, tkn_plus,
	tkn_slash,     tkn_star,
	// One or two character tokens.
	tkn_not,       tkn_neq,
	tkn_eq,        tkn_2eq,
	tkn_gt,        tkn_gteq,
	tkn_lt,        tkn_lteq,
	tkn_and,    tkn_or,
	// Literals.
	tkn_id,
	tkn_str,
	// Keywords.
	tkn_i32,    tkn_i16, tkn_i8,
	tkn_if,     tkn_else, 
	tkn_switch, tkn_break,
	tkn_while,  tkn_for, tkn_continue, 
	tkn_class,  tkn_super,
	tkn_this,   tkn_fun,
	tkn_null,   tkn_ret, 
	tkn_false,  tkn_true,
	
	tkn_err,    tkn_eof
} TokenType;

typedef struct {
	TokenType type;
	const char* start;
	uint32_t length;
	uint32_t line;
} Token;

void initScanner(const char* source);
Token scanToken(void);

#endif /* FUNVM_SCANNER_H */