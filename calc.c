// David Saju 251ADB008
// https://github.com/davidsaju/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: DeepSeek (AI), ChatGPT (AI), cppreference.com (C standard library documentation)

// AI (DeepSeek & ChatGPT): Used to discuss the overall program structure and parser design. 
// Prompt: "What is the best way to structure a recursive descent parser and tokenizer in C for arithmetic expressions?"
//   I used the high-level architectural suggestions and wrote the implementation myself.
// Resource (cppreference.com): Used for standard library function signatures (stdio.h, stdlib.h).

#include <stdio.h>
#include <stdlib.h>

// AI (DeepSeek): Prompt "How to design a tokenizer struct for a C parser to track file positions"
//   I used the idea of storing the token type and its exact byte position for error reporting.
typedef enum {
    TOK_EOF, TOK_NEWLINE, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, 
    TOK_POW, TOK_LPAREN, TOK_RPAREN, TOK_INT, TOK_FLOAT, TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    long long ival;     
    double fval;        
    size_t pos;         
} Token;

typedef struct {
    char *buffer;
    size_t length;
    size_t cursor;
} Lexer;

// Global parser state (makes it easy to pass around)
Lexer lexer;
Token current_token;

void advance() {
    current_token = next_token(&lexer);
}

// --- TOKENIZER ---
// AI (DeepSeek): Prompt "How to write a lexer that skips spaces but tracks newlines for error positions"
//   I used this to handle the space/tab skipping while keeping track of the exact \n position.
Token next_token(Lexer *lexer) {
    Token tok;
    tok.pos = lexer->cursor;
    
    while (lexer->cursor < lexer->length && 
          (lexer->buffer[lexer->cursor] == ' ' || lexer->buffer[lexer->cursor] == '\t')) {
        lexer->cursor++;
    }
    
    tok.pos = lexer->cursor; 
    
    if (lexer->cursor >= lexer->length) {
        tok.type = TOK_EOF;
        return tok;
    }

    char c = lexer->buffer[lexer->cursor];

    switch (c) {
        case '+': tok.type = TOK_PLUS; lexer->cursor++; return tok;
        case '-': tok.type = TOK_MINUS; lexer->cursor++; return tok;
        case '*': 
            if (lexer->cursor + 1 < lexer->length && lexer->buffer[lexer->cursor + 1] == '*') {
                tok.type = TOK_POW; lexer->cursor += 2; return tok;
            }
            tok.type = TOK_STAR; lexer->cursor++; return tok;
        case '/': tok.type = TOK_SLASH; lexer->cursor++; return tok;
        case '(': tok.type = TOK_LPAREN; lexer->cursor++; return tok;
        case ')': tok.type = TOK_RPAREN; lexer->cursor++; return tok;
        case '\n': tok.type = TOK_NEWLINE; lexer->cursor++; return tok;
    }

    // Handle numbers
    if (c >= '0' && c <= '9') {
        long long val = 0;
        while (lexer->cursor < lexer->length && lexer->buffer[lexer->cursor] >= '0' && lexer->buffer[lexer->cursor] <= '9') {
            val = val * 10 + (lexer->buffer[lexer->cursor] - '0');
            lexer->cursor++;
        }
        tok.type = TOK_INT;
        tok.ival = val;
        return tok;
    }

    tok.type = TOK_ERROR; 
    return tok; 
}

// --- PARSER ---
// These functions are declared here so they can call each other recursively
double parse_expr(void);
double parse_term(void);
double parse_power(void);
double parse_primary(void);

double parse_primary(void) {
    if (current_token.type == TOK_INT) {
        double val = (double)current_token.ival;
        advance();
        return val;
    }
    if (current_token.type == TOK_LPAREN) {
        advance();
        double val = parse_expr();
        if (current_token.type == TOK_RPAREN) {
            advance();
            return val;
        }
    }
    // Error handling will go here later
    return 0.0;
}

double parse_power(void) {
    double base = parse_primary();
    if (current_token.type == TOK_POW) {
        advance();
        double exp = parse_power(); // Right associative
        double result = 1.0;
        for (int i = 0; i < (int)exp; i++) result *= base;
        return result;
    }
    return base;
}

double parse_term(void) {
    double val = parse_power();
    while (current_token.type == TOK_STAR || current_token.type == TOK_SLASH) {
        TokenType op = current_token.type;
        advance();
        double right = parse_power();
        if (op == TOK_STAR) val *= right;
        else val /= right;
    }
    return val;
}

double parse_expr(void) {
    double val = parse_term();
    while (current_token.type == TOK_PLUS || current_token.type == TOK_MINUS) {
        TokenType op = current_token.type;
        advance();
        double right = parse_term();
        if (op == TOK_PLUS) val += right;
        else val -= right;
    }
    return val;
}

int main(void) {
    // For now, let's just feed it a hardcoded string to test the parser
    char test_input[] = "2 + 3 * 4\n";
    lexer.buffer = test_input;
    lexer.length = 10; // Length of the string above
    lexer.cursor = 0;
    
    advance(); // Load first token
    
    double result = parse_expr();
    printf("Result: %.15g\n", result);
    
    return 0;
}
