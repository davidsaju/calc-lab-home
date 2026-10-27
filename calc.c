// David Saju 251ADB008
// https://github.com/davidsaju/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: DeepSeek (AI), ChatGPT (AI), cppreference.com (C standard library documentation)

// AI (DeepSeek & ChatGPT): Used to discuss the overall program structure and parser design. 
// Prompt: "What is the best way to structure a recursive descent parser and tokenizer in C for arithmetic expressions?"
//   I used the high-level architectural suggestions and wrote the implementation myself.
// Resource (cppreference.com): Used for standard library function signatures (stdio.h, stdlib.h, math.h, ctype.h, dirent.h, sys/stat.h).
// AI (DeepSeek): Prompt "How to fix format truncation warnings in snprintf in C"
//   I used the advice to add precision limits (%.2000s) to the format specifiers to guarantee the compiler that the buffers won't overflow.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

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
size_t error_pos = 0; 

// Forward declarations
Token next_token(Lexer *lexer);
double parse_expr(void);
double parse_term(void);
double parse_power(void);
double parse_primary(void);

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

    if (isdigit(c)) {
        long long int_part = 0;
        while (lexer->cursor < lexer->length && isdigit(lexer->buffer[lexer->cursor])) {
            int_part = int_part * 10 + (lexer->buffer[lexer->cursor] - '0');
            lexer->cursor++;
        }

        if (lexer->cursor < lexer->length && lexer->buffer[lexer->cursor] == '.') {
            lexer->cursor++; 
            if (lexer->cursor >= lexer->length || !isdigit(lexer->buffer[lexer->cursor])) {
                tok.type = TOK_ERROR; 
                return tok;
            }
            double frac_part = 0;
            double div = 10.0;
            while (lexer->cursor < lexer->length && isdigit(lexer->buffer[lexer->cursor])) {
                frac_part += (lexer->buffer[lexer->cursor] - '0') / div;
                div *= 10.0;
                lexer->cursor++;
            }
            tok.type = TOK_FLOAT;
            tok.fval = (double)int_part + frac_part;
            return tok;
        }

        tok.type = TOK_INT;
        tok.ival = int_part;
        return tok;
    }

    tok.type = TOK_ERROR; 
    return tok; 
}

// --- PARSER ---
double parse_primary(void) {
    if (error_pos > 0) return 0.0;

    if (current_token.type == TOK_INT) {
        double val = (double)current_token.ival;
        advance();
        return val;
    }
    if (current_token.type == TOK_FLOAT) {
        double val = current_token.fval;
        advance();
        return val;
    }
    if (current_token.type == TOK_LPAREN) {
        advance();
        double val = parse_expr();
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
    if (error_pos > 0) return 0.0;

    if (current_token.type == TOK_POW) {
        advance();
        double exp = parse_power(); 
        if (error_pos > 0) return 0.0;

        if (exp < 0) {
            error_pos = current_token.pos + 1; 
            return 0.0;
        }

        double result = pow(base, exp);
        if (isinf(result) || isnan(result)) {
            error_pos = current_token.pos + 1; 
            return 0.0;
        }
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

        if (op == TOK_STAR) {
            val *= right;
        } else {
            if (right == 0.0) {
                error_pos = current_token.pos + 1; 
                return 0.0;
            }
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

// --- FILE EVALUATION ---
void evaluate_file(const char *in_path, const char *out_path) {
    FILE *f = fopen(in_path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open file %s\n", in_path);
        return;
    }
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    lexer.buffer = malloc(length + 1);
    if (fread(lexer.buffer, 1, length, f) != (size_t)length) {
        fprintf(stderr, "Error reading file\n");
        fclose(f);
        free(lexer.buffer);
        return;
    }
    lexer.buffer[length] = '\0';
    lexer.length = length;
    lexer.cursor = 0;
    fclose(f);

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Could not create output file %s\n", out_path);
        free(lexer.buffer);
        return;
    }

    advance(); 

    int at_line_start = 1;
    int in_comment = 0;

    while (current_token.type != TOK_EOF) {
        if (current_token.type == TOK_NEWLINE) {
            at_line_start = 1;
            in_comment = 0;
            advance();
            continue;
        }

        if (at_line_start && current_token.type == TOK_ERROR && lexer.buffer[current_token.pos] == '#') {
            in_comment = 1;
        }
        
        if (in_comment) {
            advance();
            continue;
        }

        at_line_start = 0;

        error_pos = 0; 
        double result = parse_expr();

        if (error_pos > 0) {
            fprintf(out, "ERROR:%zu\n", error_pos);
            fclose(out);
            free(lexer.buffer);
            return; 
        }

        if (result == (long long)result && fabs(result) < 9e18) {
            fprintf(out, "%lld\n", (long long)result);
        } else {
            fprintf(out, "%.15g\n", result);
        }

        if (current_token.type == TOK_NEWLINE) {
            at_line_start = 1;
            advance();
        } else if (current_token.type != TOK_EOF) {
            fprintf(out, "ERROR:%zu\n", current_token.pos + 1);
            fclose(out);
            free(lexer.buffer);
            return;
        }
    }

    fclose(out);
    free(lexer.buffer);
}

// --- MAIN ---
int main(int argc, char **argv) {
    const char *dir_in = NULL;
    const char *dir_out = NULL;
    const char *file_in = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 < argc) dir_in = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 < argc) dir_out = argv[++i];
        } else {
            file_in = argv[i];
        }
    }

    if (!dir_in && !file_in) {
        fprintf(stderr, "No input provided.\n");
        return 1;
    }

    // Student Details for Output Filenames
    const char *first_name = "david";
    const char *last_name = "saju";
    const char *student_id = "251adb008";

    if (dir_in) {
        char default_out[4096];
        if (!dir_out) {
            char *base = strrchr(dir_in, '/');
            base = base ? base + 1 : (char*)dir_in;
            snprintf(default_out, sizeof(default_out), "%.2000s_%s_%s", base, first_name, student_id);
            dir_out = default_out;
        }

        struct stat st = {0};
        if (stat(dir_out, &st) == -1) {
            mkdir(dir_out, 0700);
        }

        DIR *d = opendir(dir_in);
        if (!d) {
            fprintf(stderr, "Could not open directory %s\n", dir_in);
            return 1;
        }

        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            if (strstr(entry->d_name, ".txt") != NULL) {
                char in_path[4096], out_path[4096];
                snprintf(in_path, sizeof(in_path), "%.2000s/%.2000s", dir_in, entry->d_name);
                
                char base_name[4096];
                strncpy(base_name, entry->d_name, sizeof(base_name) - 1);
                base_name[sizeof(base_name) - 1] = '\0';
                char *dot = strrchr(base_name, '.');
                if (dot) *dot = '\0';

                snprintf(out_path, sizeof(out_path), "%.2000s/%.2000s_%s_%s_%s.txt", dir_out, base_name, first_name, last_name, student_id);
                evaluate_file(in_path, out_path);
            }
        }
        closedir(d);
    } else if (file_in) {
        char default_out[4096];
        if (!dir_out) {
            char *base = strrchr(file_in, '/');
            base = base ? base + 1 : (char*)file_in;
            
            char base_name[4096];
            strncpy(base_name, base, sizeof(base_name) - 1);
            base_name[sizeof(base_name) - 1] = '\0';
            char *dot = strrchr(base_name, '.');
            if (dot) *dot = '\0';
            
            snprintf(default_out, sizeof(default_out), "%.2000s_%s_%s", base_name, first_name, student_id);
            dir_out = default_out;

            struct stat st = {0};
            if (stat(dir_out, &st) == -1) {
                mkdir(dir_out, 0700);
            }
        }

        char base_name[4096];
        char *base = strrchr(file_in, '/');
        base = base ? base + 1 : (char*)file_in;
        strncpy(base_name, base, sizeof(base_name) - 1);
        base_name[sizeof(base_name) - 1] = '\0';
        char *dot = strrchr(base_name, '.');
        if (dot) *dot = '\0';

        char out_path[4096];
        snprintf(out_path, sizeof(out_path), "%.2000s/%.2000s_%s_%s_%s.txt", dir_out, base_name, first_name, last_name, student_id);
        
        evaluate_file(file_in, out_path);
    }

    return 0;
}