/**
 * @file task2.c
 * @brief Task 2 - Convert a prefix propositional-logic formula into a rooted
 *        binary parse tree.
 *
 * The prefix formula is read recursively: a binary connective is followed by
 * its left and right operands, a negation by its single operand, and an atom
 * is a leaf. The resulting tree is verified by printing it back as a fully
 * parenthesized infix formula.
 *
 * Connective symbols used: ~ for negation, + for OR, * for AND and
 * > for implication.
 *
 * Input methods supported (as per course guidelines):
 *   1. Manual prefix/infix formula from stdin (hand testing).
 *   2. DIMACS CNF benchmark file (.cnf) converted to prefix and parsed into a tree.
 *
 * Complexity: O(n) time for building, printing and freeing the tree;
 * O(h) extra memory for the recursion, where h is the tree height.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** @brief Maximum length of the input formula buffer. */
#define MAX_LEN 65536

/** @brief Maximum number of tokens for CNF to prefix conversion. */
#define MAX_TOKENS 16384

/* ---------- Parse tree node (same as before) ---------- */

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

/* ---------- Task 2: prefix -> rooted binary parse tree ---------- */

/** @brief Current read position in the prefix string while parsing. */
static const char *pp;

/**
 * @brief Test whether a character is a binary connective.
 *
 * @param c Character to test.
 * @return Non-zero if @p c is one of '+', '*' or '>', otherwise 0.
 */
static int is_binary_op(char c) { return c == '+' || c == '*' || c == '>'; }

/**
 * @brief Recursively parse one prefix formula starting at the global pointer.
 *
 * - A binary connective is followed by its left and then its right operand.
 * - '~' is followed by a single operand, which becomes its left child.
 * - Anything else is read as an atom (letters and digits).
 *
 * The program exits with an error message on an unexpected end of input or an
 * invalid symbol.
 *
 * @return Root of the parse tree for the formula that was read.
 */
static Node *parse_prefix(void) {
    while (*pp && isspace((unsigned char)*pp)) pp++;   /* skip spaces */

    if (*pp == '\0') {
        fprintf(stderr, "Error: unexpected end of prefix expression\n");
        exit(1);
    }

    /* binary connective: read left subtree, then right subtree */
    if (is_binary_op(*pp)) {
        char op[2] = { *pp, '\0' };
        pp++;
        Node *left  = parse_prefix();
        Node *right = parse_prefix();
        return new_node(op, left, right);
    }

    /* unary negation: one child (stored on the left) */
    if (*pp == '~') {
        pp++;
        Node *child = parse_prefix();
        return new_node("~", child, NULL);
    }

    /* atom: letters/digits */
    char buf[32];
    int i = 0;
    while (isalnum((unsigned char)*pp) && i < 31) buf[i++] = *pp++;
    buf[i] = '\0';
    if (i == 0) {
        fprintf(stderr, "Error: invalid symbol '%c'\n", *pp);
        exit(1);
    }
    return new_node(buf, NULL, NULL);
}

/**
 * @brief Build a rooted binary parse tree from a prefix formula.
 *
 * @param prefix Null-terminated prefix formula with space-separated tokens.
 * @return Root of the parse tree. The caller must release it with free_tree().
 */
Node *prefix_to_tree(const char *prefix) {
    pp = prefix;
    return parse_prefix();
}

/* ---------- Helper to verify the tree: print fully parenthesized infix ---------- */

/**
 * @brief Print a parse tree as a fully parenthesized infix formula.
 *
 * Atoms are printed as they are, a negation as '~' followed by its operand,
 * and every binary connective inside its own pair of brackets. The output goes
 * to stdout without a trailing newline.
 *
 * @param n Root of the (sub)tree to print; may be NULL.
 */
void print_infix(const Node *n) {
    if (!n) return;
    if (n->left == NULL && n->right == NULL) {          /* atom */
        printf("%s", n->label);
    } else if (n->right == NULL) {                      /* ~ */
        printf("~");
        print_infix(n->left);
    } else {                                            /* binary */
        printf("(");
        print_infix(n->left);
        printf("%s", n->label);
        print_infix(n->right);
        printf(")");
    }
}

/* ======================================================================
 *  INPUT METHODS & CNF CONVERSION HELPERS
 * ====================================================================== */

typedef struct {
    char text[32];
} Token;

static int is_connective(char c) {
    return c == '~' || c == '*' || c == '+' || c == '>';
}

static int connective_precedence(char c) {
    switch (c) {
        case '~': return 4;
        case '*': return 3;
        case '+': return 2;
        case '>': return 1;
        default:  return 0;
    }
}

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
            s++;
        }
    }
    return count;
}

static void infix_to_prefix_helper(const char *infix, char *prefix_out) {
    Token *tokens = (Token *)malloc(MAX_TOKENS * sizeof(Token));
    Token *out = (Token *)malloc(MAX_TOKENS * sizeof(Token));
    char *op_stack = (char *)malloc(MAX_TOKENS * sizeof(char));
    if (!tokens || !out || !op_stack) {
        fprintf(stderr, "Out of memory in conversion\n");
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
                top--;
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
            strncpy(out[out_count].text, t, sizeof(out[out_count].text) - 1);
            out[out_count].text[sizeof(out[out_count].text) - 1] = '\0';
            out_count++;
        }
    }

    while (top > 0) {
        if (op_stack[top - 1] != ')') {
            out[out_count].text[0] = op_stack[top - 1];
            out[out_count].text[1] = '\0';
            out_count++;
        }
        top--;
    }

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
 * @brief Input Method 1: Read prefix formula directly from stdin.
 *
 * @param buffer Output buffer.
 * @param max_len Maximum length of buffer.
 * @return 0 on success, -1 on EOF or error.
 */
int read_input_prefix(char *buffer, size_t max_len) {
    if (!fgets(buffer, (int)max_len, stdin)) return -1;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 0;
}

/**
 * @brief Input Method 2: Read DIMACS CNF file and convert to prefix notation.
 *
 * @param filename File path to .cnf file.
 * @param prefix_out Output buffer receiving space-separated prefix tokens.
 * @param max_len Capacity of output buffer.
 * @return 0 on success, -1 on failure.
 */
int read_input_cnf_as_prefix(const char *filename, char *prefix_out, size_t max_len) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening CNF file");
        return -1;
    }

    char *infix = (char *)malloc(max_len);
    if (!infix) { fclose(fp); return -1; }
    infix[0] = '\0';

    char line[4096];
    int num_vars = 0, num_clauses = 0;
    char current_clause[8192] = "";
    int lits_in_clause = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *ptr = line;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0' || *ptr == 'c') continue;
        if (*ptr == 'p') {
            sscanf(ptr, "p cnf %d %d", &num_vars, &num_clauses);
            continue;
        }

        char *token = strtok(ptr, " \t\r\n");
        while (token) {
            int lit = atoi(token);
            if (lit == 0) {
                if (lits_in_clause > 0) {
                    if (infix[0] == '\0') {
                        strncpy(infix, current_clause, max_len - 1);
                        infix[max_len - 1] = '\0';
                    } else {
                        size_t needed = strlen(infix) + strlen(current_clause) + 6;
                        if (needed < max_len) {
                            char *temp = (char *)malloc(max_len);
                            if (temp) {
                                snprintf(temp, max_len, "(%s*%s)", infix, current_clause);
                                strncpy(infix, temp, max_len - 1);
                                infix[max_len - 1] = '\0';
                                free(temp);
                            }
                        } else {
                            fclose(fp);
                            infix_to_prefix_helper(infix, prefix_out);
                            free(infix);
                            return 0;
                        }
                    }
                    current_clause[0] = '\0';
                    lits_in_clause = 0;
                }
            } else {
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
    infix_to_prefix_helper(infix, prefix_out);
    free(infix);
    return 0;
}

/* ---------- Main ---------- */

/**
 * @brief Program entry point: read a prefix formula (or CNF file), build its
 *        parse tree and print the tree back as a fully parenthesized infix formula.
 *
 * @param argc Argument count.
 * @param argv Argument vector (argv[1] can optionally specify a .cnf file).
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    static char prefix[MAX_LEN];

    if (argc > 1) {
        printf("Reading DIMACS CNF benchmark file: %s\n", argv[1]);
        if (read_input_cnf_as_prefix(argv[1], prefix, sizeof(prefix)) != 0) {
            fprintf(stderr, "Failed to load CNF file.\n");
            return 1;
        }
    } else {
        printf("Select input method:\n");
        printf("  1. Enter prefix formula manually (e.g. * + p q ~ r)\n");
        printf("  2. Read from DIMACS CNF file\n");
        printf("Enter choice (1 or 2, default 1) or prefix formula directly:\n");

        char choice_buf[MAX_LEN];
        if (!fgets(choice_buf, sizeof(choice_buf), stdin)) return 1;
        choice_buf[strcspn(choice_buf, "\r\n")] = '\0';

        if (strcmp(choice_buf, "2") == 0) {
            char filepath[512];
            printf("Enter path to DIMACS .cnf file: ");
            if (!fgets(filepath, sizeof(filepath), stdin)) return 1;
            filepath[strcspn(filepath, "\r\n")] = '\0';
            if (read_input_cnf_as_prefix(filepath, prefix, sizeof(prefix)) != 0) {
                fprintf(stderr, "Failed to load CNF file.\n");
                return 1;
            }
        } else if (strcmp(choice_buf, "1") == 0) {
            printf("Enter a prefix formula (e.g. * + p q ~ r):\n");
            if (read_input_prefix(prefix, sizeof(prefix)) != 0) return 1;
        } else {
            /* Piped or direct input */
            strncpy(prefix, choice_buf, sizeof(prefix) - 1);
            prefix[sizeof(prefix) - 1] = '\0';
        }
    }

    if (strlen(prefix) == 0) {
        fprintf(stderr, "Error: empty input formula.\n");
        return 1;
    }

    /* If user entered an infix formula starting with '(', convert it to prefix */
    char *trimmed = prefix;
    while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
    if (*trimmed == '(') {
        static char conv_buf[MAX_LEN];
        infix_to_prefix_helper(trimmed, conv_buf);
        strncpy(prefix, conv_buf, sizeof(prefix) - 1);
        prefix[sizeof(prefix) - 1] = '\0';
    }

    printf("\nPrefix : %s\n", prefix);
    Node *root = prefix_to_tree(prefix);

    printf("Infix  : ");
    print_infix(root);
    printf("\n");

    free_tree(root);
    return 0;
}
