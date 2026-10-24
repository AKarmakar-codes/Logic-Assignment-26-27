/**
 * @file task4.c
 * @brief Task 4 - Height of a propositional-logic parse tree, computed
 *        recursively and iteratively.
 *
 * Height convention: number of EDGES on the longest root-to-leaf path
 * (a single atom has height 0). The number of levels = height + 1.
 *
 * Input : a fully parenthesized INFIX formula (~ negation, + OR, * AND, > implication)
 * Output: the height of its parse tree, computed by both methods.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** @brief Maximum length of the input formula buffer. */
#define MAX_LEN 65536

/* ---------- Parse tree node (same as task1.c / task2.c) ---------- */

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

/* ---------- Infix -> parse tree (parser from task1.c) ---------- */

/**
 * @brief Recursively parse a fully parenthesized infix formula.
 *
 * Reads from the global input pointer @c p. The grammar is:
 * @code
 * formula := atom | '~' formula | '(' formula [op formula] ')'
 * op      := '+' | '*' | '>'
 * @endcode
 *
 * @return Root of the parse tree built for the formula that was read.
 */
Node *parse_formula(void) {
    skip_spaces();

    if (*p == '(') {
        p++;
        Node *left = parse_formula();
        skip_spaces();
        if (*p == ')') {
            p++;
            return left;
        }
        char op[2] = { *p, '\0' };
        p++;
        Node *right = parse_formula();
        skip_spaces();
        if (*p == ')') p++;
        return new_node(op, left, right);
    }

    if (*p == '~') {
        p++;
        Node *child = parse_formula();
        return new_node("~", child, NULL);
    }

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
 * @brief Compute the height of a parse tree, recursively.
 *
 * Uses height(NULL) = -1, height(leaf) = 0 and
 * height(n) = 1 + max(height(left), height(right)).
 *
 * Time O(n); extra memory O(h) for the call stack, where h is the height.
 *
 * @param n Root of the (sub)tree; may be NULL.
 * @return Height of the tree in edges, or -1 for an empty tree.
 */
int height_rec(const Node *n) {
    if (!n) return -1;
    int hl = height_rec(n->left);
    int hr = height_rec(n->right);
    return 1 + (hl > hr ? hl : hr);
}

/**
 * @brief Compute the height of a parse tree, iteratively (level-order / BFS).
 *
 * Processes the tree one level at a time using a queue; the number of levels
 * visited minus one is the height. The queue is a heap array that doubles
 * in size when full.
 *
 * Time O(n); extra memory O(w), where w is the maximum width of the tree.
 *
 * @param root Root of the tree; may be NULL.
 * @return Height of the tree in edges, or -1 for an empty tree.
 */
int height_iter(const Node *root) {
    if (!root) return -1;

    int cap = 64, head = 0, tail = 0;
    const Node **queue = (const Node **)malloc(cap * sizeof(const Node *));
    if (!queue) { fprintf(stderr, "Out of memory\n"); exit(1); }

    queue[tail++] = root;
    int height = -1;

    while (head < tail) {
        int level_size = tail - head;       /* nodes on the current level */
        height++;
        for (int i = 0; i < level_size; i++) {
            const Node *n = queue[head++];
            int need = (n->left ? 1 : 0) + (n->right ? 1 : 0);
            if (tail + need > cap) {        /* grow the queue if needed */
                cap *= 2;
                queue = (const Node **)realloc(queue, cap * sizeof(const Node *));
                if (!queue) { fprintf(stderr, "Out of memory\n"); exit(1); }
            }
            if (n->left)  queue[tail++] = n->left;
            if (n->right) queue[tail++] = n->right;
        }
    }
    free(queue);
    return height;
}

/* ======================================================================
 *  INPUT METHODS (Instructor Specification)
 *  Method 1: Read infix notation from stdin (hand/manual testing)
 *  Method 2: Read DIMACS CNF file (.cnf) and convert to infix notation
 * ====================================================================== */

/**
 * @brief Input Method 1: Read a propositional logic formula in infix notation from stdin.
 *
 * @param buffer Output buffer.
 * @param max_len Maximum length of buffer.
 * @return 0 on success, -1 on EOF or error.
 */
int read_input_infix(char *buffer, size_t max_len) {
    if (!fgets(buffer, (int)max_len, stdin)) return -1;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 0;
}

/**
 * @brief Input Method 2: Read DIMACS CNF file and convert to fully parenthesized infix notation.
 *
 * @param filename File path to .cnf file.
 * @param buffer Output buffer receiving the infix formula.
 * @param max_len Capacity of output buffer.
 * @return 0 on success, -1 on failure.
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
                            fclose(fp);
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
    return 0;
}

/* ---------- Main ---------- */

/**
 * @brief Program entry point: read an infix formula (or CNF file), build its parse tree and
 *        print its height computed recursively and iteratively.
 *
 * @param argc Argument count.
 * @param argv Argument vector (argv[1] can optionally specify a .cnf file).
 * @return 0 on success, 1 if no input could be read.
 */
int main(int argc, char *argv[]) {
    static char infix[MAX_LEN];

    if (argc > 1) {
        printf("Reading DIMACS CNF benchmark file: %s\n", argv[1]);
        if (read_input_cnf(argv[1], infix, sizeof(infix)) != 0) {
            fprintf(stderr, "Failed to load CNF file: %s\n", argv[1]);
            return 1;
        }
    } else {
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
        fprintf(stderr, "Error: empty input formula.\n");
        return 1;
    }

    printf("\nInfix formula:\n%s\n", infix);
    Node *root = build_parse_tree(infix);

    printf("\nHeight of the parse tree is the longest path from root to a leaf (in edges)\n");
    printf("Height (recursive): %d\n", height_rec(root));
    printf("Height (iterative): %d\n", height_iter(root));

    free_tree(root);
    return 0;
}