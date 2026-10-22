/**
 * @file task5.c
 * @brief Task 5 - Evaluate a propositional-logic formula bottom-up on its
 *        parse tree and produce the complete truth table.
 *
 * Input : a fully parenthesized INFIX formula (~ negation, + OR, * AND, > implication)
 * Output: the truth table of the formula, plus whether it is a tautology,
 *         a contradiction, or merely satisfiable.
 *
 * The Node structure and the infix -> parse tree parser are the same as in
 * task1.c, so they are reused here unchanged.
 *
 * Complexity: for a formula with n nodes and k distinct atoms, the table has
 * 2^k rows and each row is evaluated in O(n) time, so the total time is
 * O(2^k * n). The evaluation uses O(h) call-stack space, where h is the tree
 * height, plus O(k) for the atom table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** @brief Maximum length of the input formula buffer. */
#define MAX_LEN   1024

/** @brief Maximum number of distinct atoms allowed (the table has 2^k rows). */
#define MAX_ATOMS 20      /* 2^20 rows is the practical limit for a printed table */

/* ---------- Parse tree node (same as task1.c) ---------- */

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
static int is_atom(const Node *n)     { return n->left == NULL && n->right == NULL; }

/**
 * @brief Test whether a node is a negation (exactly one child).
 *
 * @param n Node to test; must not be NULL.
 * @return Non-zero if @p n has only a left child, otherwise 0.
 */
static int is_negation(const Node *n) { return n->left != NULL && n->right == NULL; }

/* ======================================================================
 *  TASK 5 - ATOMS AND EVALUATION
 * ====================================================================== */

/**
 * @brief Distinct propositional atoms of the formula, in order of first appearance.
 */
typedef struct {
    char names[MAX_ATOMS][32];   /**< Atom names; names[i] is the i-th distinct atom */
    int  count;                  /**< Number of distinct atoms stored so far */
} AtomTable;

/**
 * @brief Collect the distinct atoms of the tree (pre-order walk).
 *
 * Each atom is added to the table the first time it is seen.
 *
 * @param n Root of the (sub)tree to scan; may be NULL.
 * @param t Atom table to fill; @c t->count must be 0 before the first call.
 * @return 0 on success, -1 if there are more than MAX_ATOMS distinct atoms.
 */
int collect_atoms(const Node *n, AtomTable *t) {
    if (!n) return 0;
    if (is_atom(n)) {
        for (int i = 0; i < t->count; i++)
            if (strcmp(t->names[i], n->label) == 0) return 0;   /* already known */
        if (t->count == MAX_ATOMS) return -1;
        strcpy(t->names[t->count++], n->label);
        return 0;
    }
    if (collect_atoms(n->left, t) < 0) return -1;
    return collect_atoms(n->right, t);
}

/**
 * @brief Find the index of an atom in the atom table.
 *
 * @param t    Atom table to search.
 * @param name Atom name to look for.
 * @return Index of the atom in @p t, or -1 if it is absent.
 */
static int atom_index(const AtomTable *t, const char *name) {
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->names[i], name) == 0) return i;
    return -1;
}

/**
 * @brief Evaluate the formula bottom-up (post-order).
 *
 * The values of the children are computed first, then the connective at the
 * node combines them: '*' is AND, '+' is OR, '~' is NOT and '>' is
 * implication (computed as NOT a OR b).
 *
 * Time O(n) per assignment; extra memory O(h) for the call stack.
 *
 * @param n   Root of the (sub)tree; must not be NULL.
 * @param t   Atom table giving the position of every atom.
 * @param val Truth values (0 or 1); val[i] is the value of atom t->names[i].
 * @return Truth value (0 or 1) of the subtree.
 */
int evaluate(const Node *n, const AtomTable *t, const int *val) {
    if (is_atom(n))
        return val[atom_index(t, n->label)];

    if (is_negation(n))
        return !evaluate(n->left, t, val);

    int a = evaluate(n->left, t, val);      /* children first ... */
    int b = evaluate(n->right, t, val);     /* ... then the connective */

    switch (n->label[0]) {
        case '*': return a && b;            /* AND */
        case '+': return a || b;            /* OR */
        case '>': return !a || b;           /* implication */
        default:
            fprintf(stderr, "Error: unknown connective '%s'\n", n->label);
            exit(1);
    }
}

/**
 * @brief Print the complete truth table of a formula.
 *
 * Prints 2^k rows for k atoms, ordered from all-true down to all-false, and
 * then a summary: the number of true rows and whether the formula is a
 * tautology, a contradiction, or only satisfiable.
 *
 * @param root    Root of the formula's parse tree.
 * @param t       Atom table of the formula (see collect_atoms()).
 * @param formula Original formula text, shown in the last column header.
 */
void print_truth_table(const Node *root, const AtomTable *t, const char *formula) {
    int k = t->count;
    long rows = 1L << k;
    long true_rows = 0;
    int val[MAX_ATOMS];

    /* header */
    for (int i = 0; i < k; i++) printf(" %-5s|", t->names[i]);
    printf(" %s\n", formula);
    for (int i = 0; i < k; i++) printf("-------+");
    printf("-------\n");

    for (long r = 0; r < rows; r++) {
        for (int i = 0; i < k; i++)
            val[i] = !((r >> (k - 1 - i)) & 1);     /* first row: all true */

        int result = evaluate(root, t, val);
        true_rows += result;

        for (int i = 0; i < k; i++) printf(" %-5c|", val[i] ? 'T' : 'F');
        printf(" %c\n", result ? 'T' : 'F');
    }

    printf("\nRows evaluating to true: %ld of %ld\n", true_rows, rows);
    if (true_rows == rows)      printf("The formula is a TAUTOLOGY (valid).\n");
    else if (true_rows == 0)    printf("The formula is a CONTRADICTION (unsatisfiable).\n");
    else                        printf("The formula is SATISFIABLE but not valid.\n");
}

/* ---------- Main ---------- */

/**
 * @brief Program entry point: read an infix formula, build its parse tree and
 *        print its truth table.
 *
 * @return 0 on success, 1 if no input could be read or the formula has more
 *         than MAX_ATOMS distinct atoms.
 */
int main(void) {
    char infix[MAX_LEN];

    printf("Enter a fully parenthesized formula (~ negation, + OR, * AND, > implication):\n");
    if (!fgets(infix, sizeof(infix), stdin)) return 1;
    infix[strcspn(infix, "\n")] = '\0';

    Node *root = build_parse_tree(infix);

    AtomTable t;
    t.count = 0;
    if (collect_atoms(root, &t) < 0) {
        fprintf(stderr, "Error: more than %d distinct atoms; truth table too large.\n", MAX_ATOMS);
        free_tree(root);
        return 1;
    }

    printf("\nAtoms (%d): ", t.count);
    for (int i = 0; i < t.count; i++) printf("%s ", t.names[i]);
    printf("\n\n");

    print_truth_table(root, &t, infix);

    free_tree(root);
    return 0;
}