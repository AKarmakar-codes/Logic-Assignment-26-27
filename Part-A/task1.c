#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEN 1024

/* ---------- Parse tree node ---------- */
typedef struct Node {
    char label[32];        /* atom name or connective: ~ + * > */
    struct Node *left;     /* only child for '~' */
    struct Node *right;    /* NULL for '~' and atoms */
} Node;

static const char *p;      /* current position in input */

/* ---------- Helpers ---------- */
static void skip_spaces(void) {
    while (*p && isspace((unsigned char)*p)) p++;
}

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

Node *build_parse_tree(const char *infix) {
    p = infix;
    return parse_formula();
}

/* ---------- Task 1: infix -> prefix ---------- */
/* Pre-order traversal: node, then left, then right */
void tree_to_prefix(const Node *n, char *out) {
    if (!n) return;
    strcat(out, n->label);
    strcat(out, " ");
    tree_to_prefix(n->left, out);
    tree_to_prefix(n->right, out);
}

void infix_to_prefix(const char *infix, char *prefix_out) {
    Node *root = build_parse_tree(infix);
    prefix_out[0] = '\0';
    tree_to_prefix(root, prefix_out);

    /* trim trailing space */
    size_t len = strlen(prefix_out);
    if (len > 0 && prefix_out[len - 1] == ' ') prefix_out[len - 1] = '\0';
}

/* ---------- Memory cleanup ---------- */
void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ---------- Main ---------- */
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