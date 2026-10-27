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
    long long ival;     // For integers
    double fval;        // For floats
    size_t pos;         // Exact position in the file (0-based, we add 1 when printing)
} Token;

// This struct holds the whole file in memory so we don't slow down on large files
typedef struct {
    char *buffer;
    size_t length;
    size_t cursor;
} Lexer;

// AI (DeepSeek): Prompt "How to write a lexer that skips spaces but tracks newlines for error positions"
//   I used this to handle the space/tab skipping while keeping track of the exact \n position.
Token next_token(Lexer *lexer) {
    Token tok;
    tok.pos = lexer->cursor;
    
    // Skip spaces and tabs
    while (lexer->cursor < lexer->length && 
          (lexer->buffer[lexer->cursor] == ' ' || lexer->buffer[lexer->cursor] == '\t')) {
        lexer->cursor++;
    }
    
    tok.pos = lexer->cursor; // Update position after spaces
    
    if (lexer->cursor >= lexer->length) {
        tok.type = TOK_EOF;
        return tok;
    }

    char c = lexer->buffer[lexer->cursor];

    // Handle single-character tokens
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

    // If we get here, it's a number or an error. 
    // (We'll handle numbers in the next step, for now just mark it as ERROR if it's not a digit)
    tok.type = TOK_ERROR; 
    return tok; 
}

int main(void) {
    printf("Hello, calc!\n");
    return 0;
}
