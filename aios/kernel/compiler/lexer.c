#include "lexer.h"

static int is_alpha(char c) {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';
}
static int is_digit(char c) { return c>='0'&&c<='9'; }
static int is_alnum(char c) { return is_alpha(c)||is_digit(c); }
static int is_space(char c) { return c==' '||c=='\t'||c=='\r'||c=='\n'; }

static void str_copy_n(char* dst, const char* src, int n) {
    int i=0;
    while(i<n-1&&src[i]){dst[i]=src[i];i++;}
    dst[i]=0;
}
static int str_eq(const char* a, const char* b) {
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}
    return *a==*b;
}

static token_type_t keyword(const char* s) {
    if(str_eq(s,"print"))  return TOK_PRINT;
    if(str_eq(s,"let"))    return TOK_LET;
    if(str_eq(s,"if"))     return TOK_IF;
    if(str_eq(s,"else"))   return TOK_ELSE;
    if(str_eq(s,"while"))  return TOK_WHILE;
    if(str_eq(s,"fn"))     return TOK_FN;
    if(str_eq(s,"return")) return TOK_RETURN;
    if(str_eq(s,"ai"))     return TOK_AI;
    return TOK_IDENT;
}

static void add_tok(lexer_t* l, token_type_t t, const char* val) {
    if(l->count>=MAX_TOKENS) return;
    l->tokens[l->count].type = t;
    l->tokens[l->count].line = l->line;
    str_copy_n(l->tokens[l->count].value, val, MAX_TOKEN_LEN);
    l->count++;
}

void lexer_init(lexer_t* l, const char* src) {
    l->src=src; l->pos=0; l->line=1; l->count=0; l->error=0;
}

int lexer_tokenize(lexer_t* l) {
    const char* s = l->src;
    int i = 0;
    while(s[i]) {
        // Skip whitespace
        if(is_space(s[i])) {
            if(s[i]=='\n') l->line++;
            i++; continue;
        }
        // Skip comments
        if(s[i]=='/'&&s[i+1]=='/') {
            while(s[i]&&s[i]!='\n') i++;
            continue;
        }
        // Number
        if(is_digit(s[i])) {
            int start=i;
            while(is_digit(s[i])) i++;
            char buf[MAX_TOKEN_LEN];
            str_copy_n(buf, s+start, i-start+1);
            buf[i-start]=0;
            add_tok(l, TOK_NUMBER, buf);
            continue;
        }
        // String
        if(s[i]=='"') {
            i++;
            int start=i;
            while(s[i]&&s[i]!='"') i++;
            char buf[MAX_TOKEN_LEN];
            str_copy_n(buf, s+start, i-start+1);
            buf[i-start]=0;
            if(s[i]=='"') i++;
            add_tok(l, TOK_STRING, buf);
            continue;
        }
        // Identifier or keyword
        if(is_alpha(s[i])) {
            int start=i;
            while(is_alnum(s[i])) i++;
            char buf[MAX_TOKEN_LEN];
            str_copy_n(buf, s+start, i-start+1);
            buf[i-start]=0;
            add_tok(l, keyword(buf), buf);
            continue;
        }
        // Operators
        char c=s[i];
        if(c=='='&&s[i+1]=='='){add_tok(l,TOK_EQEQ,"==");i+=2;continue;}
        if(c=='!'&&s[i+1]=='='){add_tok(l,TOK_NEQ,"!=");i+=2;continue;}
        if(c=='<'&&s[i+1]=='='){add_tok(l,TOK_LTEQ,"<=");i+=2;continue;}
        if(c=='>'&&s[i+1]=='='){add_tok(l,TOK_GTEQ,">=");i+=2;continue;}
        if(c=='&'&&s[i+1]=='&'){add_tok(l,TOK_AND,"&&");i+=2;continue;}
        if(c=='|'&&s[i+1]=='|'){add_tok(l,TOK_OR,"||");i+=2;continue;}
        switch(c) {
            case '+': add_tok(l,TOK_PLUS,"+");   break;
            case '-': add_tok(l,TOK_MINUS,"-");  break;
            case '*': add_tok(l,TOK_STAR,"*");   break;
            case '/': add_tok(l,TOK_SLASH,"/");  break;
            case '=': add_tok(l,TOK_EQ,"=");     break;
            case '<': add_tok(l,TOK_LT,"<");     break;
            case '>': add_tok(l,TOK_GT,">");     break;
            case '!': add_tok(l,TOK_NOT,"!");    break;
            case '(': add_tok(l,TOK_LPAREN,"("); break;
            case ')': add_tok(l,TOK_RPAREN,")"); break;
            case '{': add_tok(l,TOK_LBRACE,"{"); break;
            case '}': add_tok(l,TOK_RBRACE,"}"); break;
            case ';': add_tok(l,TOK_SEMI,";");   break;
            case ',': add_tok(l,TOK_COMMA,",");  break;
            case ':': add_tok(l,TOK_COLON,":");  break;
        }
        i++;
    }
    add_tok(l, TOK_EOF, "");
    return l->count;
}

const char* token_name(token_type_t t) {
    switch(t) {
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_IDENT:  return "IDENT";
        case TOK_PRINT:  return "print";
        case TOK_LET:    return "let";
        case TOK_IF:     return "if";
        case TOK_WHILE:  return "while";
        case TOK_FN:     return "fn";
        case TOK_RETURN: return "return";
        case TOK_AI:     return "ai";
        case TOK_EOF:    return "EOF";
        default:         return "?";
    }
}
