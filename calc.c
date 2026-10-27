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
#include <string.h>

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

Lexer lexer;
Token current_token;

// Forward declaration so advance() knows next_token exists
Token next_token(Lexer *lexer);

// If an error occurs, we store its 1-based position here.
// If it's 0, no error has occurred.
size_t error_pos = 0; 

void advance() {
    current_token = next_token(&lexer);
}

// --- TOKENIZER ---
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
// AI (DeepSeek): Prompt "How to implement exact error positions in a recursive descent parser for math expressions"
//   I used the advice to set a global error_pos using the current token's 1-based position (tok.pos + 1).
double parse_expr(void);
double parse_term(void);
double parse_power(void);
double parse_primary(void);

double parse_primary(void) {
    if (error_pos > 0) return 0.0;

    if (current_token.type == TOK_INT) {
        double val = (double)current_token.ival;
        advance();
        return val;
    }
    
    if (current_token.type == TOK_LPAREN) {
        advance();
        double val = parse_expr();
        
        // Check for missing closing parenthesis
        if (current_token.type != TOK_RPAREN) {
            error_pos = current_token.pos + 1;
            return 0.0;
        }
        
        advance();
        return val;
    }

    error_pos = current_token.pos + 1;
    return 0.0;
}

double parse_power(void) {
    if (error_pos > 0) return 0.0;

    double base = parse_primary();
    if (current_token.type == TOK_POW) {
        advance();
        double exp = parse_power(); 
        double result = 1.0;
        for (int i = 0; i < (int)exp; i++) result *= base;
        return result;
    }
    return base;
}

double parse_term(void) {
    if (error_pos > 0) return 0.0;

    double val = parse_power();
    while (current_token.type == TOK_STAR || current_token.type == TOK_SLASH) {
        TokenType op = current_token.type;
        advance();
        double right = parse_power();
        if (error_pos > 0) return 0.0;

        if (op == TOK_STAR) val *= right;
        else {
            // Division by zero check will go here later
            val /= right;
        }
    }
    return val;
}

double parse_expr(void) {
    if (error_pos > 0) return 0.0;

    double val = parse_term();
    while (current_token.type == TOK_PLUS || current_token.type == TOK_MINUS) {
        TokenType op = current_token.type;
        advance();
        double right = parse_term();
        if (error_pos > 0) return 0.0;

        if (op == TOK_PLUS) val += right;
        else val -= right;
    }
    return val;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.txt>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    lexer.buffer = malloc(length + 1);
    if (fread(lexer.buffer, 1, length, f) != (size_t)length) {
        fprintf(stderr, "Error reading file\n");
        return 1;
    }
    lexer.buffer[length] = '\0';
    lexer.length = length;
    lexer.cursor = 0;
    fclose(f);

    advance(); // Load first token

    while (current_token.type != TOK_EOF) {
        // Skip blank lines
        if (current_token.type == TOK_NEWLINE) {
            advance();
            continue;
        }

        error_pos = 0; // Reset for this line
        double result = parse_expr();

        if (error_pos > 0) {
            printf("ERROR:%zu\n", error_pos);
            free(lexer.buffer);
            return 0; 
        }

        printf("%.15g\n", result);

        if (current_token.type == TOK_NEWLINE) {
            advance();
        } else if (current_token.type != TOK_EOF) {
            printf("ERROR:%zu\n", current_token.pos + 1);
            free(lexer.buffer);
            return 0;
        }
    }

    free(lexer.buffer);
    return 0;
}
