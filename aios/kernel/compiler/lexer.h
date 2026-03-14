#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKENS     512
#define MAX_TOKEN_LEN  64

typedef enum {
    // Literals
    TOK_NUMBER, TOK_STRING, TOK_IDENT,
    // Keywords
    TOK_PRINT, TOK_LET, TOK_IF, TOK_ELSE,
    TOK_WHILE, TOK_FN, TOK_RETURN, TOK_AI,
    // Operators
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
    TOK_EQ, TOK_EQEQ, TOK_NEQ, TOK_LT, TOK_GT,
    TOK_LTEQ, TOK_GTEQ, TOK_AND, TOK_OR, TOK_NOT,
    // Delimiters
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_SEMI, TOK_COMMA, TOK_COLON,
    // Special
    TOK_EOF, TOK_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    char         value[MAX_TOKEN_LEN];
    int          line;
} token_t;

typedef struct {
    const char* src;
    int         pos;
    int         line;
    token_t     tokens[MAX_TOKENS];
    int         count;
    int         error;
    char        errmsg[128];
} lexer_t;

void lexer_init(lexer_t* l, const char* src);
int  lexer_tokenize(lexer_t* l);
const char* token_name(token_type_t t);

#endif
