/**
 * @file task1.c
 * @brief Task 1 - Convert a fully parenthesized infix propositional-logic
 *        formula into a prefix formula.
 *
 * The infix formula is first parsed into a binary parse tree. A pre-order
 * traversal (node, left subtree, right subtree) of that tree then yields the
 * prefix expression.
 *
 * Connective symbols used: ~ for negation, + for OR, * for AND and
 * > for implication.
 *
 * Input : a well-formed, fully parenthesized infix formula, read from stdin.
 * Output: the equivalent prefix formula, with tokens separated by spaces.
 *
 * Complexity: parsing, traversal and clean-up are each O(n) in the number of
 * characters/nodes; the output string uses O(n) memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** @brief Maximum length of the input formula buffer. */
#define MAX_LEN 1024

/* ---------- Parse tree node ---------- */

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

/* ---------- Helpers ---------- */

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

/* ---------- Parse tree construction ----------
   Grammar (fully parenthesized):
     formula := atom | '~' formula | '(' formula [op formula] ')'
     op      := '+' | '*' | '>'
*/

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

/* ---------- Task 1: infix -> prefix ---------- */

/**
 * @brief Append the prefix form of a tree to a string (pre-order traversal).
 *
 * Visits the node first, then its left subtree, then its right subtree. Every
 * label is followed by a single space.
 *
 * @param n   Root of the (sub)tree to traverse; may be NULL.
 * @param out Output buffer to append to; it must be large enough and
 *            null-terminated before the first call.
 */
void tree_to_prefix(const Node *n, char *out) {
    if (!n) return;
    strcat(out, n->label);
    strcat(out, " ");
    tree_to_prefix(n->left, out);
    tree_to_prefix(n->right, out);
}

/**
 * @brief Convert an infix formula into the equivalent prefix formula.
 *
 * Builds the parse tree, writes its pre-order traversal to @p prefix_out and
 * removes the trailing space.
 *
 * @param infix      Null-terminated, fully parenthesized infix formula.
 * @param prefix_out Output buffer receiving the prefix formula (tokens
 *                   separated by single spaces); it must be large enough.
 */
void infix_to_prefix(const char *infix, char *prefix_out) {
    Node *root = build_parse_tree(infix);
    prefix_out[0] = '\0';
    tree_to_prefix(root, prefix_out);

    /* trim trailing space */
    size_t len = strlen(prefix_out);
    if (len > 0 && prefix_out[len - 1] == ' ') prefix_out[len - 1] = '\0';
}

/* ---------- Memory cleanup ---------- */

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

/* ---------- Main ---------- */

/**
 * @brief Program entry point: read an infix formula and print its prefix form.
 *
 * @return 0 on success, 1 if no input could be read.
 */
int main(void) {
    char infix[MAX_LEN];
    char prefix[MAX_LEN * 2];

    printf("Enter a fully parenthesized formula (~ negation, + OR, * AND, > implication):\n");
    if (!fgets(infix, sizeof(infix), stdin)) return 1;
    infix[strcspn(infix, "\n")] = '\0';

    infix_to_prefix(infix, prefix);
    printf("Prefix: %s\n", prefix);
    return 0;
}