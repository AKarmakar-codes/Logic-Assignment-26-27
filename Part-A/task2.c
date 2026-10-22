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
 * Input : a well-formed prefix formula with space-separated tokens, read from
 *         stdin (for example: * + p q ~ r).
 * Output: the corresponding fully parenthesized infix formula.
 *
 * Complexity: O(n) time for building, printing and freeing the tree;
 * O(h) extra memory for the recursion, where h is the tree height.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** @brief Maximum length of the input formula buffer. */
#define MAX_LEN 1024

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

/* ---------- Main ---------- */

/**
 * @brief Program entry point: read a prefix formula, build its parse tree and
 *        print the tree back as a fully parenthesized infix formula.
 *
 * @return 0 on success, 1 if no input could be read.
 */
int main(void) {
    char prefix[MAX_LEN];

    printf("Enter a prefix formula (e.g. * + p q ~ r):\n");
    if (!fgets(prefix, sizeof(prefix), stdin)) return 1;
    prefix[strcspn(prefix, "\n")] = '\0';

    Node *root = prefix_to_tree(prefix);

    printf("Infix : ");
    print_infix(root);
    printf("\n");

    free_tree(root);
    return 0;
}