#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEN 1024

/* ---------- Parse tree node (same as before) ---------- */
typedef struct Node {
    char label[32];        /* atom name or connective: ~ + * > */
    struct Node *left;     /* only child for '~' */
    struct Node *right;    /* NULL for '~' and atoms */
} Node;

static Node *new_node(const char *label, Node *l, Node *r) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    strncpy(n->label, label, sizeof(n->label) - 1);
    n->label[sizeof(n->label) - 1] = '\0';
    n->left = l;
    n->right = r;
    return n;
}

void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ---------- Task 2: prefix -> rooted binary parse tree ---------- */
static const char *pp;     /* current position in prefix string */

static int is_binary_op(char c) { return c == '+' || c == '*' || c == '>'; }

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

Node *prefix_to_tree(const char *prefix) {
    pp = prefix;
    return parse_prefix();
}

/* ---------- Helper to verify the tree: print fully parenthesized infix ---------- */
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