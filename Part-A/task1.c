/**
 * @file task1.c
 * @brief Task 1 - Convert a fully parenthesized infix propositional-logic
 *        formula into a prefix formula using an operator stack.
 *
 * Implements Task 1 of Assignment 1.
 * Connective symbols used: ~ for negation, + for OR, * for AND and
 * > for implication.
 *
 * Reference for infix-to-prefix using operator stack:
 * http://scanftree.com/Data_Structure/infix-to-prefix
 *
 * Input methods supported (as per course guidelines):
 *   1. Manual infix formula entered from stdin (hand testing).
 *   2. DIMACS CNF benchmark file (.cnf) converted into fully parenthesized infix.
 *
 * Complexity: O(n) time and O(n) auxiliary memory for operator stack conversion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** @brief Maximum length of the input formula buffer. */
#define MAX_LEN 65536

/** @brief Maximum number of tokens in a formula for stack processing. */
#define MAX_TOKENS 16384

/* ======================================================================
 *  PARSE TREE STRUCTURE (Retained for AST interoperability & Task 2)
 * ====================================================================== */

/**
 * @brief A node of the propositional-logic parse tree.
 *
 * Atoms are leaves (both children NULL). A negation has exactly one child,
 * stored in @c left. Binary connectives have both children.
 */
typedef struct Node {
    char label[32];        /**< Atom name or connective: ~ + * > */
    struct Node *left;     /**< Left child; the only child for '~' */
    struct Node *right;    /**< Right child; NULL for '~' and atoms */
} Node;

/** @brief Current read position in the input string while parsing. */
static const char *p;

/**
 * @brief Advance the global input pointer past any whitespace.
 */
static void skip_spaces(void) {
    while (*p && isspace((unsigned char)*p)) p++;
}

/**
 * @brief Allocate and initialise a new parse tree node.
 *
 * Exits the program with an error message if memory allocation fails.
 *
 * @param label Atom name or connective symbol (truncated to 31 characters).
 * @param l     Left child, or NULL.
 * @param r     Right child, or NULL.
 * @return Pointer to the newly created node.
 */
static Node *new_node(const char *label, Node *l, Node *r) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    strncpy(n->label, label, sizeof(n->label) - 1);
    n->label[sizeof(n->label) - 1] = '\0';
    n->left = l;
    n->right = r;
    return n;
}

/**
 * @brief Recursively parse a fully parenthesized infix formula into an AST.
 *
 * Grammar (fully parenthesized):
 *   formula := atom | '~' formula | '(' formula [op formula] ')'
 *   op      := '+' | '*' | '>'
 *
 * @return Root of the parse tree built for the formula that was read.
 */
Node *parse_formula(void) {
    skip_spaces();

    if (*p == '(') {
        p++;                                /* consume '(' */
        Node *left = parse_formula();
        skip_spaces();
        if (*p == ')') {                    /* e.g. (p) or (~p) */
            p++;
            return left;
        }
        char op[2] = { *p, '\0' };          /* + * > */
        p++;
        Node *right = parse_formula();
        skip_spaces();
        if (*p == ')') p++;                 /* consume ')' */
        return new_node(op, left, right);
    }

    if (*p == '~') {
        p++;
        Node *child = parse_formula();
        return new_node("~", child, NULL);
    }

    /* atom: letters/digits, e.g. p, q1, x */
    char buf[32];
    int i = 0;
    while (isalnum((unsigned char)*p) && i < 31) buf[i++] = *p++;
    buf[i] = '\0';
    return new_node(buf, NULL, NULL);
}

/**
 * @brief Build the parse tree of an infix formula.
 *
 * @param infix Null-terminated, fully parenthesized infix formula.
 * @return Root of the parse tree. The caller must release it with free_tree().
 */
Node *build_parse_tree(const char *infix) {
    p = infix;
    return parse_formula();
}

/**
 * @brief Append the prefix form of a tree to a string (pre-order traversal).
 *
 * @param n   Root of the (sub)tree to traverse; may be NULL.
 * @param out Output buffer to append to; it must be null-terminated beforehand.
 */
void tree_to_prefix(const Node *n, char *out) {
    if (!n) return;
    strcat(out, n->label);
    strcat(out, " ");
    tree_to_prefix(n->left, out);
    tree_to_prefix(n->right, out);
}

/**
 * @brief Free a parse tree and all of its nodes (post-order).
 *
 * @param n Root of the tree to free; may be NULL.
 */
void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ======================================================================
 *  TASK 1: OPERATOR-STACK INFIX TO PREFIX CONVERSION
 *  Reference: http://scanftree.com/Data_Structure/infix-to-prefix
 * ====================================================================== */

/**
 * @brief Token container for propositional logic formulas.
 */
typedef struct {
    char text[32]; /**< Token string: atom identifier, connective, or bracket */
} Token;

/**
 * @brief Check whether a character is a supported logical connective.
 *
 * @param c Character to inspect.
 * @return 1 if connective (~, *, +, >), 0 otherwise.
 */
static int is_connective(char c) {
    return c == '~' || c == '*' || c == '+' || c == '>';
}

/**
 * @brief Return operator precedence for connectives.
 *
 * Precedence hierarchy:
 *   - '~' (unary negation)  : 4 (highest)
 *   - '*' (binary AND)      : 3
 *   - '+' (binary OR)       : 2
 *   - '>' (implication)     : 1 (lowest)
 *
 * @param c Operator symbol.
 * @return Integer precedence value.
 */
static int connective_precedence(char c) {
    switch (c) {
        case '~': return 4;
        case '*': return 3;
        case '+': return 2;
        case '>': return 1;
        default:  return 0;
    }
}

/**
 * @brief Tokenize a propositional logic formula into discrete tokens.
 *
 * @param str Input formula string.
 * @param tokens Array of Token structures to populate.
 * @param max_tokens Maximum token capacity of the array.
 * @return Total number of tokens extracted.
 */
static int tokenize_formula(const char *str, Token *tokens, int max_tokens) {
    int count = 0;
    const char *s = str;
    while (*s && count < max_tokens) {
        while (*s && isspace((unsigned char)*s)) s++;
        if (!*s) break;
        if (*s == '(' || *s == ')' || is_connective(*s)) {
            tokens[count].text[0] = *s++;
            tokens[count].text[1] = '\0';
            count++;
        } else if (isalnum((unsigned char)*s)) {
            int j = 0;
            while (isalnum((unsigned char)*s) && j < 31) {
                tokens[count].text[j++] = *s++;
            }
            tokens[count].text[j] = '\0';
            count++;
        } else {
            s++; /* Ignore any extraneous characters */
        }
    }
    return count;
}

/**
 * @brief Convert an infix formula into prefix notation using an operator stack.
 *
 * Follows the standard ScanfTree infix-to-prefix algorithm:
 *   1. Tokenize formula and scan tokens from right to left.
 *   2. Operands (atoms) are appended directly to the output.
 *   3. Closing parentheses ')' are pushed to the operator stack.
 *   4. Opening parentheses '(' trigger popping operators until matching ')' is found.
 *   5. Operators are pushed according to precedence.
 *   6. Reversing the collected output tokens produces the final prefix expression.
 *
 * @param infix Null-terminated, fully parenthesized infix formula.
 * @param prefix_out Output buffer receiving space-separated prefix tokens.
 */
void infix_to_prefix_stack(const char *infix, char *prefix_out) {
    Token *tokens = (Token *)malloc(MAX_TOKENS * sizeof(Token));
    Token *out = (Token *)malloc(MAX_TOKENS * sizeof(Token));
    char *op_stack = (char *)malloc(MAX_TOKENS * sizeof(char));
    if (!tokens || !out || !op_stack) {
        fprintf(stderr, "Error: out of memory in infix_to_prefix_stack\n");
        exit(1);
    }

    int n = tokenize_formula(infix, tokens, MAX_TOKENS);
    int top = 0;
    int out_count = 0;

    for (int i = n - 1; i >= 0; i--) {
        char *t = tokens[i].text;
        if (strcmp(t, ")") == 0) {
            op_stack[top++] = ')';
        } else if (strcmp(t, "(") == 0) {
            while (top > 0 && op_stack[top - 1] != ')') {
                out[out_count].text[0] = op_stack[--top];
                out[out_count].text[1] = '\0';
                out_count++;
            }
            if (top > 0 && op_stack[top - 1] == ')') {
                top--; /* Pop matching ')' */
            }
        } else if (is_connective(t[0]) && t[1] == '\0') {
            char op = t[0];
            while (top > 0 && op_stack[top - 1] != ')' &&
                   connective_precedence(op_stack[top - 1]) > connective_precedence(op)) {
                out[out_count].text[0] = op_stack[--top];
                out[out_count].text[1] = '\0';
                out_count++;
            }
            op_stack[top++] = op;
        } else {
            /* Atom / Operand */
            strncpy(out[out_count].text, t, sizeof(out[out_count].text) - 1);
            out[out_count].text[sizeof(out[out_count].text) - 1] = '\0';
            out_count++;
        }
    }

    /* Pop any remaining operators from the stack */
    while (top > 0) {
        if (op_stack[top - 1] != ')') {
            out[out_count].text[0] = op_stack[top - 1];
            out[out_count].text[1] = '\0';
            out_count++;
        }
        top--;
    }

    /* Reverse output tokens to produce prefix ordering */
    prefix_out[0] = '\0';
    size_t cur_len = 0;
    for (int i = out_count - 1; i >= 0; i--) {
        size_t tlen = strlen(out[i].text);
        if (cur_len > 0) {
            prefix_out[cur_len++] = ' ';
            prefix_out[cur_len] = '\0';
        }
        strcat(prefix_out, out[i].text);
        cur_len += tlen;
    }

    free(tokens);
    free(out);
    free(op_stack);
}

/**
 * @brief Task 1 Interface: Convert infix propositional logic to prefix.
 *
 * Uses the operator-stack method (ScanfTree reference) for primary conversion.
 *
 * @param infix Null-terminated, fully parenthesized infix formula.
 * @param prefix_out Output buffer receiving space-separated prefix tokens.
 */
void infix_to_prefix(const char *infix, char *prefix_out) {
    infix_to_prefix_stack(infix, prefix_out);
}

/* ======================================================================
 *  INPUT METHODS (Instructor Specification)
 *  Method 1: Read infix notation from stdin (hand/manual testing)
 *  Method 2: Read DIMACS CNF file (.cnf) and convert to infix notation
 * ====================================================================== */

/**
 * @brief Input Method 1: Read a propositional logic formula in infix notation from stdin.
 *
 * Allows hand-testing of smaller inputs entered by the user. Trims newlines.
 *
 * @param buffer Output buffer to receive the formula string.
 * @param max_len Maximum capacity of the buffer.
 * @return 0 on success, -1 on EOF or read failure.
 */
int read_input_infix(char *buffer, size_t max_len) {
    if (!fgets(buffer, (int)max_len, stdin)) return -1;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 0;
}

/**
 * @brief Input Method 2: Read a DIMACS CNF file and convert it into fully
 *        parenthesized infix notation.
 *
 * In accordance with DIMACS CNF specifications:
 *   - Lines starting with 'c' are comments.
 *   - The problem line starts with 'p cnf <variables> <clauses>'.
 *   - Literals within a clause are combined with OR ('+').
 *   - Clauses are combined with AND ('*').
 *   - Negative numbers represent negated literals (~xK).
 *
 * @param filename File path to the DIMACS CNF benchmark file.
 * @param buffer Output buffer to receive the fully parenthesized infix formula.
 * @param max_len Maximum capacity of the output buffer.
 * @return 0 on success, -1 if the file could not be read or opened.
 */
int read_input_cnf(const char *filename, char *buffer, size_t max_len) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening CNF file");
        return -1;
    }

    char line[4096];
    int num_vars = 0, num_clauses = 0;
    buffer[0] = '\0';

    char current_clause[8192] = "";
    int lits_in_clause = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *ptr = line;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0' || *ptr == 'c') continue; /* Comment / blank */
        if (*ptr == 'p') {
            sscanf(ptr, "p cnf %d %d", &num_vars, &num_clauses);
            continue;
        }

        char *token = strtok(ptr, " \t\r\n");
        while (token) {
            int lit = atoi(token);
            if (lit == 0) {
                /* End of clause */
                if (lits_in_clause > 0) {
                    if (buffer[0] == '\0') {
                        strncpy(buffer, current_clause, max_len - 1);
                        buffer[max_len - 1] = '\0';
                    } else {
                        size_t needed = strlen(buffer) + strlen(current_clause) + 6;
                        if (needed < max_len) {
                            char *temp = (char *)malloc(max_len);
                            if (temp) {
                                snprintf(temp, max_len, "(%s*%s)", buffer, current_clause);
                                strncpy(buffer, temp, max_len - 1);
                                buffer[max_len - 1] = '\0';
                                free(temp);
                            }
                        } else {
                            /* Buffer limit reached: return safely */
                            fclose(fp);
                            return 0;
                        }
                    }
                    current_clause[0] = '\0';
                    lits_in_clause = 0;
                }
            } else {
                /* Literal representation */
                char lit_str[64];
                if (lit < 0) {
                    snprintf(lit_str, sizeof(lit_str), "(~x%d)", -lit);
                } else {
                    snprintf(lit_str, sizeof(lit_str), "x%d", lit);
                }

                if (lits_in_clause == 0) {
                    strncpy(current_clause, lit_str, sizeof(current_clause) - 1);
                    current_clause[sizeof(current_clause) - 1] = '\0';
                } else {
                    char temp[8192];
                    snprintf(temp, sizeof(temp), "(%s+%s)", current_clause, lit_str);
                    strncpy(current_clause, temp, sizeof(current_clause) - 1);
                    current_clause[sizeof(current_clause) - 1] = '\0';
                }
                lits_in_clause++;
            }
            token = strtok(NULL, " \t\r\n");
        }
    }

    fclose(fp);
    return 0;
}

/* ======================================================================
 *  MAIN ENTRY POINT
 * ====================================================================== */

/**
 * @brief Program entry point for Task 1.
 *
 * Supports both manual stdin input and DIMACS CNF file reading, as well
 * as direct command-line arguments and piped input.
 *
 * @param argc Argument count.
 * @param argv Argument vector (argv[1] can optionally specify a .cnf file).
 * @return 0 on success, non-zero on error.
 */
int main(int argc, char *argv[]) {
    static char infix[MAX_LEN];
    static char prefix[MAX_LEN * 2];

    if (argc > 1) {
        /* Direct command-line argument: Treat argv[1] as a DIMACS CNF file */
        printf("Reading DIMACS CNF benchmark file: %s\n", argv[1]);
        if (read_input_cnf(argv[1], infix, sizeof(infix)) != 0) {
            fprintf(stderr, "Failed to read CNF file: %s\n", argv[1]);
            return 1;
        }
    } else {
        /* Interactive selection menu */
        printf("Select input method:\n");
        printf("  1. Enter infix formula manually\n");
        printf("  2. Read from DIMACS CNF file\n");
        printf("Enter choice (1 or 2, default 1) or formula directly:\n");

        char choice_buf[MAX_LEN];
        if (!fgets(choice_buf, sizeof(choice_buf), stdin)) return 1;
        choice_buf[strcspn(choice_buf, "\r\n")] = '\0';

        if (strcmp(choice_buf, "2") == 0) {
            char filepath[512];
            printf("Enter path to DIMACS .cnf file: ");
            if (!fgets(filepath, sizeof(filepath), stdin)) return 1;
            filepath[strcspn(filepath, "\r\n")] = '\0';
            if (read_input_cnf(filepath, infix, sizeof(infix)) != 0) {
                fprintf(stderr, "Failed to load CNF file.\n");
                return 1;
            }
        } else if (strcmp(choice_buf, "1") == 0) {
            printf("Enter a fully parenthesized formula (~ negation, + OR, * AND, > implication):\n");
            if (read_input_infix(infix, sizeof(infix)) != 0) return 1;
        } else {
            /* Piped input or direct formula entry */
            strncpy(infix, choice_buf, sizeof(infix) - 1);
            infix[sizeof(infix) - 1] = '\0';
        }
    }

    if (strlen(infix) == 0) {
        fprintf(stderr, "Error: empty formula received.\n");
        return 1;
    }

    printf("\nInfix  : %s\n", infix);
    infix_to_prefix(infix, prefix);
    printf("Prefix : %s\n", prefix);

    return 0;
}