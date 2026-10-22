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

Node *build_parse_tree(const char *infix) {
    p = infix;
    return parse_formula();
}

/* ---------- Node type helpers ---------- */
static int is_atom(const Node *n)   { return n->left == NULL && n->right == NULL; }
static int is_negation(const Node *n) { return n->left != NULL && n->right == NULL; }

/* ======================================================================
 *  RECURSIVE TRAVERSALS
 * ====================================================================== */

/** In-order (infix, brackets omitted), recursive. */
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

/** Pre-order (prefix): node, left, right. Recursive. */
void preorder_rec(const Node *n) {
    if (!n) return;
    printf("%s ", n->label);
    preorder_rec(n->left);
    preorder_rec(n->right);
}

/** Post-order (postfix): left, right, node. Recursive. */
void postorder_rec(const Node *n) {
    if (!n) return;
    postorder_rec(n->left);
    postorder_rec(n->right);
    printf("%s ", n->label);
}

/* ======================================================================
 *  SIMPLE DYNAMIC STACK (so deep trees do not overflow a fixed array)
 * ====================================================================== */
typedef struct {
    const Node *node;
    int state;             /* used only by the iterative in-order traversal */
} Frame;

typedef struct {
    Frame *data;
    int top, cap;
} Stack;

static void stack_init(Stack *s) {
    s->cap = 64;
    s->top = 0;
    s->data = (Frame *)malloc(s->cap * sizeof(Frame));
    if (!s->data) { fprintf(stderr, "Out of memory\n"); exit(1); }
}

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

static int  stack_empty(const Stack *s) { return s->top == 0; }
static void stack_free(Stack *s)        { free(s->data); }

/* ======================================================================
 *  ITERATIVE TRAVERSALS
 * ====================================================================== */

/**
 * In-order (infix, brackets omitted), iterative.
 * Each stack frame remembers how far we got with that node:
 *   state 0 : just arrived  -> go to the left child
 *   state 1 : left is done  -> print the label, go to the right child
 *   state 2 : right is done -> pop
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

/** Pre-order, iterative: pop, visit, push right then left. */
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
 * Post-order, iterative (two-stack method):
 * stack 1 produces "node, right, left" order; stack 2 reverses it
 * into "left, right, node".
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

/* ---------- Main ---------- */
int main(void) {
    char infix[MAX_LEN];

    printf("Enter a fully parenthesized formula (~ negation, + OR, * AND, > implication):\n");
    if (!fgets(infix, sizeof(infix), stdin)) return 1;
    infix[strcspn(infix, "\n")] = '\0';

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
