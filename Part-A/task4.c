/**
 * @file task4.c
 * @brief Task 4 - Height of a propositional-logic parse tree, computed
 *        recursively and iteratively.
 *
 * Height convention: number of EDGES on the longest root-to-leaf path
 * (a single atom has height 0). The number of levels = height + 1.
 *
 * Input : a fully parenthesized INFIX formula (~ negation, + OR, * AND, > implication)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEN 1024

/* ---------- Parse tree node (same as task1.c / task2.c) ---------- */
typedef struct Node {
    char label[32];        /* atom name or connective: ~ + * > */
    struct Node *left;     /* only child for '~' */
    struct Node *right;    /* NULL for '~' and atoms */
} Node;

static const char *p;      /* current position in input */

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

void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ---------- Infix -> parse tree (parser from task1.c) ---------- */
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

Node *build_parse_tree(const char *infix) {
    p = infix;
    return parse_formula();
}

/* ======================================================================
 *  HEIGHT - RECURSIVE
 *  height(NULL) = -1, height(leaf) = 0,
 *  height(n)    = 1 + max(height(left), height(right))
 *  Time O(n), extra memory O(h) call stack.
 * ====================================================================== */
int height_rec(const Node *n) {
    if (!n) return -1;
    int hl = height_rec(n->left);
    int hr = height_rec(n->right);
    return 1 + (hl > hr ? hl : hr);
}

/* ======================================================================
 *  HEIGHT - ITERATIVE (level-order / BFS using a queue)
 *  Process the tree one level at a time; the number of levels visited
 *  minus one is the height.
 *  Time O(n), extra memory O(w) where w is the maximum width of the tree.
 * ====================================================================== */
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

/* ---------- Main ---------- */
int main(void) {
    char infix[MAX_LEN];

    printf("Enter a fully parenthesized formula (~ negation, + OR, * AND, > implication):\n");
    if (!fgets(infix, sizeof(infix), stdin)) return 1;
    infix[strcspn(infix, "\n")] = '\0';

    Node *root = build_parse_tree(infix);

    printf("Height (recursive): %d\n", height_rec(root));
    printf("Height (iterative): %d\n", height_iter(root));

    free_tree(root);
    return 0;
}
