/**
 * @file task3.c
 * @brief Task 3 - Recursive and iterative in-order, pre-order and post-order
 *        traversals of a propositional-logic parse tree.
 *
 * Input : a fully parenthesized INFIX formula (~ negation, + OR, * AND, > implication)
 * Output: in-order (infix), pre-order (prefix) and post-order (postfix) of the
 *         parse tree, each produced recursively AND iteratively.
 *
 * The tree is built with the same parser used in task1.c, so the Node
 * structure is identical to the one used in task1.c / task2.c.
 *
 * Complexity: every traversal visits each node once, so all six run in O(n)
 * time. The recursive versions use O(h) call-stack space, where h is the tree
 * height. The iterative versions use an explicit stack of O(h) frames, except
 * the two-stack post-order, which needs O(n) space.
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

/* ---------- Node type helpers ---------- */

/**
 * @brief Test whether a node is an atom (a leaf).
 *
 * @param n Node to test; must not be NULL.
 * @return Non-zero if @p n has no children, otherwise 0.
 */
static int is_atom(const Node *n)   { return n->left == NULL && n->right == NULL; }

/**
 * @brief Test whether a node is a negation (exactly one child).
 *
 * @param n Node to test; must not be NULL.
 * @return Non-zero if @p n has only a left child, otherwise 0.
 */
static int is_negation(const Node *n) { return n->left != NULL && n->right == NULL; }

/* ======================================================================
 *  RECURSIVE TRAVERSALS
 * ====================================================================== */

/**
 * @brief In-order (infix, brackets omitted) traversal, recursive.
 *
 * Prints the left subtree, the node and then the right subtree. A negation
 * is printed as "~ " followed by its operand, and binary connectives are
 * surrounded by spaces.
 *
 * @param n Root of the (sub)tree to traverse; may be NULL.
 */
void inorder_rec(const Node *n) {
    if (!n) return;
    if (is_atom(n)) {
        printf("%s", n->label);
    } else if (is_negation(n)) {
        printf("~ ");
        inorder_rec(n->left);
    } else {
        inorder_rec(n->left);
        printf(" %s ", n->label);
        inorder_rec(n->right);
    }
}

/**
 * @brief Pre-order (prefix) traversal, recursive: node, left, right.
 *
 * @param n Root of the (sub)tree to traverse; may be NULL.
 */
void preorder_rec(const Node *n) {
    if (!n) return;
    printf("%s ", n->label);
    preorder_rec(n->left);
    preorder_rec(n->right);
}

/**
 * @brief Post-order (postfix) traversal, recursive: left, right, node.
 *
 * @param n Root of the (sub)tree to traverse; may be NULL.
 */
void postorder_rec(const Node *n) {
    if (!n) return;
    postorder_rec(n->left);
    postorder_rec(n->right);
    printf("%s ", n->label);
}

/* ======================================================================
 *  SIMPLE DYNAMIC STACK (so deep trees do not overflow a fixed array)
 * ====================================================================== */

/**
 * @brief One entry of the traversal stack.
 */
typedef struct {
    const Node *node;      /**< Tree node held by this frame */
    int state;             /**< Progress marker; used only by the iterative in-order traversal */
} Frame;

/**
 * @brief A growable stack of Frame entries.
 */
typedef struct {
    Frame *data;           /**< Heap array holding the frames */
    int top, cap;          /**< Number of frames on the stack (top) and allocated capacity in frames (cap) */
} Stack;

/**
 * @brief Initialise an empty stack with an initial capacity of 64 frames.
 *
 * Exits the program with an error message if allocation fails.
 *
 * @param s Stack to initialise.
 */
static void stack_init(Stack *s) {
    s->cap = 64;
    s->top = 0;
    s->data = (Frame *)malloc(s->cap * sizeof(Frame));
    if (!s->data) { fprintf(stderr, "Out of memory\n"); exit(1); }
}

/**
 * @brief Push a frame onto the stack, doubling the capacity when it is full.
 *
 * Exits the program with an error message if reallocation fails. Pointers
 * into @c s->data obtained before this call may become invalid.
 *
 * @param s     Stack to push onto.
 * @param n     Tree node to store in the new frame.
 * @param state Initial state value of the new frame.
 */
static void stack_push(Stack *s, const Node *n, int state) {
    if (s->top == s->cap) {
        s->cap *= 2;
        s->data = (Frame *)realloc(s->data, s->cap * sizeof(Frame));
        if (!s->data) { fprintf(stderr, "Out of memory\n"); exit(1); }
    }
    s->data[s->top].node = n;
    s->data[s->top].state = state;
    s->top++;
}

/**
 * @brief Test whether a stack is empty.
 *
 * @param s Stack to test.
 * @return Non-zero if the stack holds no frames, otherwise 0.
 */
static int  stack_empty(const Stack *s) { return s->top == 0; }

/**
 * @brief Release the memory held by a stack.
 *
 * @param s Stack to free.
 */
static void stack_free(Stack *s)        { free(s->data); }

/* ======================================================================
 *  ITERATIVE TRAVERSALS
 * ====================================================================== */

/**
 * @brief In-order (infix, brackets omitted) traversal, iterative.
 *
 * Each stack frame remembers how far we got with that node:
 *   - state 0 : just arrived  -> go to the left child
 *   - state 1 : left is done  -> print the label, go to the right child
 *   - state 2 : right is done -> pop
 *
 * Produces the same output as inorder_rec().
 *
 * @param root Root of the tree to traverse; may be NULL.
 */
void inorder_iter(const Node *root) {
    if (!root) return;
    Stack s;
    stack_init(&s);
    stack_push(&s, root, 0);

    while (!stack_empty(&s)) {
        Frame *f = &s.data[s.top - 1];
        const Node *n = f->node;

        if (is_atom(n)) {
            printf("%s", n->label);
            s.top--;
        } else if (is_negation(n)) {
            if (f->state == 0) {
                printf("~ ");
                f->state = 1;
                stack_push(&s, n->left, 0);     /* f may be invalid after this */
            } else {
                s.top--;
            }
        } else {                                /* binary connective */
            if (f->state == 0) {
                f->state = 1;
                stack_push(&s, n->left, 0);
            } else if (f->state == 1) {
                printf(" %s ", n->label);
                f->state = 2;
                stack_push(&s, n->right, 0);
            } else {
                s.top--;
            }
        }
    }
    stack_free(&s);
}

/**
 * @brief Pre-order traversal, iterative: pop, visit, push right then left.
 *
 * Produces the same output as preorder_rec().
 *
 * @param root Root of the tree to traverse; may be NULL.
 */
void preorder_iter(const Node *root) {
    if (!root) return;
    Stack s;
    stack_init(&s);
    stack_push(&s, root, 0);

    while (!stack_empty(&s)) {
        const Node *n = s.data[--s.top].node;
        printf("%s ", n->label);
        if (n->right) stack_push(&s, n->right, 0);
        if (n->left)  stack_push(&s, n->left, 0);
    }
    stack_free(&s);
}

/**
 * @brief Post-order traversal, iterative (two-stack method).
 *
 * The first stack produces the order "node, right, left"; the second stack
 * reverses it into "left, right, node". Produces the same output as
 * postorder_rec().
 *
 * @param root Root of the tree to traverse; may be NULL.
 */
void postorder_iter(const Node *root) {
    if (!root) return;
    Stack s1, s2;
    stack_init(&s1);
    stack_init(&s2);
    stack_push(&s1, root, 0);

    while (!stack_empty(&s1)) {
        const Node *n = s1.data[--s1.top].node;
        stack_push(&s2, n, 0);
        if (n->left)  stack_push(&s1, n->left, 0);
        if (n->right) stack_push(&s1, n->right, 0);
    }
    while (!stack_empty(&s2)) {
        printf("%s ", s2.data[--s2.top].node->label);
    }
    stack_free(&s1);
    stack_free(&s2);
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
 *        print all six traversals (recursive and iterative).
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

    printf("\nIn-order   (recursive): "); inorder_rec(root);    printf("\n");
    printf("In-order   (iterative): "); inorder_iter(root);   printf("\n");
    printf("Pre-order  (recursive): "); preorder_rec(root);   printf("\n");
    printf("Pre-order  (iterative): "); preorder_iter(root);  printf("\n");
    printf("Post-order (recursive): "); postorder_rec(root);  printf("\n");
    printf("Post-order (iterative): "); postorder_iter(root); printf("\n");

    free_tree(root);
    return 0;
}