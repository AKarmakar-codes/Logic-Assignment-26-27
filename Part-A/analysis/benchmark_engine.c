/**
 * @file benchmark_engine.c
 * @brief High-precision empirical performance and memory benchmarking engine
 *        for CS F214 Logic Assignment Part A (Tasks 1 to 5).
 *
 * Measures:
 * 1. Task 1 (Infix to Prefix) and Task 2 (Prefix to Tree) conversion time vs. N.
 * 2. Task 3 Traversals: Recursive vs. Iterative for In-Order, Pre-Order, Post-Order.
 * 3. Task 4 Height: Recursive (DFS) vs. Iterative (BFS / Stack).
 * 4. Task 5 Truth Table: Exponential scaling O(2^k * N) for k = 2..18.
 * 5. Memory Footprint: Heap nodes, Call-stack depth, and Iterative stack space.
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#define MAX_BUFFER 65536
#define MAX_ATOMS  25

/* ---------- Parse Tree Data Structure ---------- */

typedef struct Node {
    char label[32];
    struct Node *left;
    struct Node *right;
} Node;

static const char *p;

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

static void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

static int is_atom(const Node *n)     { return n->left == NULL && n->right == NULL; }
static int is_negation(const Node *n) { return n->left != NULL && n->right == NULL; }

/* ---------- Parser (Infix -> Tree) ---------- */

static Node *parse_formula(void) {
    skip_spaces();
    if (*p == '(') {
        p++;
        Node *left = parse_formula();
        skip_spaces();
        if (*p == ')') { p++; return left; }
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

static Node *build_parse_tree(const char *infix) {
    p = infix;
    return parse_formula();
}

/* ---------- Task 1: Infix to Prefix ---------- */

static void tree_to_prefix_buf(const Node *n, char *out) {
    if (!n) return;
    strcat(out, n->label);
    strcat(out, " ");
    tree_to_prefix_buf(n->left, out);
    tree_to_prefix_buf(n->right, out);
}

static void infix_to_prefix_bench(const char *infix, char *prefix_out) {
    Node *root = build_parse_tree(infix);
    prefix_out[0] = '\0';
    tree_to_prefix_buf(root, prefix_out);
    free_tree(root);
}

/* ---------- Task 2: Prefix to Tree ---------- */

static Node *prefix_to_tree_parse(void) {
    skip_spaces();
    if (!*p) return NULL;
    if (*p == '~') {
        p++;
        return new_node("~", prefix_to_tree_parse(), NULL);
    }
    if (*p == '+' || *p == '*' || *p == '>') {
        char op[2] = { *p, '\0' };
        p++;
        Node *left = prefix_to_tree_parse();
        Node *right = prefix_to_tree_parse();
        return new_node(op, left, right);
    }
    char buf[32];
    int i = 0;
    while (*p && !isspace((unsigned char)*p) && i < 31) buf[i++] = *p++;
    buf[i] = '\0';
    return new_node(buf, NULL, NULL);
}

static Node *build_tree_from_prefix(const char *prefix) {
    p = prefix;
    return prefix_to_tree_parse();
}

/* ---------- Metrics: Node Counting & Depth ---------- */

static int count_nodes(const Node *n) {
    if (!n) return 0;
    return 1 + count_nodes(n->left) + count_nodes(n->right);
}

/* ---------- Task 3: Traversals (Pure Computation Benchmarking) ---------- */

static volatile unsigned long g_checksum = 0;

/* 1. In-order Recursive */
static void bench_inorder_rec(const Node *n) {
    if (!n) return;
    bench_inorder_rec(n->left);
    g_checksum += (unsigned char)n->label[0];
    bench_inorder_rec(n->right);
}

/* 2. In-order Iterative */
static void bench_inorder_iter(const Node *root) {
    const Node *stack[MAX_BUFFER];
    int top = -1;
    const Node *curr = root;
    while (curr != NULL || top >= 0) {
        while (curr != NULL) {
            stack[++top] = curr;
            curr = curr->left;
        }
        curr = stack[top--];
        g_checksum += (unsigned char)curr->label[0];
        curr = curr->right;
    }
}

/* 3. Pre-order Recursive */
static void bench_preorder_rec(const Node *n) {
    if (!n) return;
    g_checksum += (unsigned char)n->label[0];
    bench_preorder_rec(n->left);
    bench_preorder_rec(n->right);
}

/* 4. Pre-order Iterative */
static void bench_preorder_iter(const Node *root) {
    if (!root) return;
    const Node *stack[MAX_BUFFER];
    int top = -1;
    stack[++top] = root;
    while (top >= 0) {
        const Node *n = stack[top--];
        g_checksum += (unsigned char)n->label[0];
        if (n->right) stack[++top] = n->right;
        if (n->left)  stack[++top] = n->left;
    }
}

/* 5. Post-order Recursive */
static void bench_postorder_rec(const Node *n) {
    if (!n) return;
    bench_postorder_rec(n->left);
    bench_postorder_rec(n->right);
    g_checksum += (unsigned char)n->label[0];
}

/* 6. Post-order Iterative (Two-stack method) */
static void bench_postorder_iter(const Node *root) {
    if (!root) return;
    const Node *s1[MAX_BUFFER];
    const Node *s2[MAX_BUFFER];
    int top1 = -1, top2 = -1;
    s1[++top1] = root;
    while (top1 >= 0) {
        const Node *curr = s1[top1--];
        s2[++top2] = curr;
        if (curr->left)  s1[++top1] = curr->left;
        if (curr->right) s1[++top1] = curr->right;
    }
    while (top2 >= 0) {
        const Node *curr = s2[top2--];
        g_checksum += (unsigned char)curr->label[0];
    }
}

/* ---------- Task 4: Tree Height ---------- */

static int height_rec(const Node *n) {
    if (!n || is_atom(n)) return 0;
    if (is_negation(n)) return 1 + height_rec(n->left);
    int lh = height_rec(n->left);
    int rh = height_rec(n->right);
    return 1 + (lh > rh ? lh : rh);
}

typedef struct QItem {
    const Node *node;
    int depth;
} QItem;

static int height_iter(const Node *root) {
    if (!root || is_atom(root)) return 0;
    static QItem queue[MAX_BUFFER];
    int front = 0, back = 0;
    int max_depth = 0;

    queue[back++] = (QItem){ root, 0 };
    while (front < back) {
        QItem item = queue[front++];
        const Node *curr = item.node;
        if (is_atom(curr)) {
            if (item.depth > max_depth) max_depth = item.depth;
        } else {
            if (curr->left)  queue[back++] = (QItem){ curr->left, item.depth + 1 };
            if (curr->right) queue[back++] = (QItem){ curr->right, item.depth + 1 };
        }
    }
    return max_depth;
}

/* ---------- Task 5: Truth Table Evaluation ---------- */

typedef struct AtomTable {
    char names[MAX_ATOMS][32];
    int count;
} AtomTable;

static int atom_index(const AtomTable *t, const char *name) {
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->names[i], name) == 0) return i;
    return -1;
}

static int collect_atoms(const Node *n, AtomTable *t) {
    if (!n) return 0;
    if (is_atom(n)) {
        if (atom_index(t, n->label) < 0) {
            if (t->count >= MAX_ATOMS) return -1;
            strncpy(t->names[t->count], n->label, 31);
            t->names[t->count][31] = '\0';
            t->count++;
        }
        return 0;
    }
    if (collect_atoms(n->left, t) < 0) return -1;
    return collect_atoms(n->right, t);
}

static int evaluate_node(const Node *n, const AtomTable *t, const int *val) {
    if (is_atom(n))
        return val[atom_index(t, n->label)];
    if (is_negation(n))
        return !evaluate_node(n->left, t, val);

    int a = evaluate_node(n->left, t, val);
    int b = evaluate_node(n->right, t, val);

    switch (n->label[0]) {
        case '*': return a && b;
        case '+': return a || b;
        case '>': return !a || b;
        default: return 0;
    }
}

static long long bench_truth_table(const Node *root, const AtomTable *t) {
    int k = t->count;
    long long total_rows = 1LL << k;
    int val[MAX_ATOMS];
    long long true_count = 0;

    for (long long row = 0; row < total_rows; row++) {
        for (int i = 0; i < k; i++)
            val[i] = (int)((row >> (k - 1 - i)) & 1);
        if (evaluate_node(root, t, val)) true_count++;
    }
    return true_count;
}

/* ---------- Timing Utilities ---------- */

static double get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

/* ---------- Benchmark Suite Execution ---------- */

/* Static buffers to prevent stack overflow */
static char g_infix[MAX_BUFFER];
static char g_prefix_buf[MAX_BUFFER * 2];
static char g_temp_pool[512][MAX_BUFFER];
static char g_skewed[MAX_BUFFER];
static char g_balanced[MAX_BUFFER];
static char g_f_str[MAX_BUFFER];

int main(void) {
    printf("Starting CS F214 Benchmark Engine...\n");

    FILE *fp_json = fopen("Part-A/analysis/data/benchmark_results.json", "w");
    FILE *fp_csv_trav = fopen("Part-A/analysis/data/benchmark_traversals.csv", "w");
    FILE *fp_csv_tt = fopen("Part-A/analysis/data/benchmark_truth_table.csv", "w");
    FILE *fp_csv_height = fopen("Part-A/analysis/data/benchmark_height.csv", "w");
    FILE *fp_csv_scaling = fopen("Part-A/analysis/data/benchmark_scaling.csv", "w");

    if (!fp_json || !fp_csv_trav || !fp_csv_tt || !fp_csv_height || !fp_csv_scaling) {
        fprintf(stderr, "Failed to create output files.\n");
        return 1;
    }

    fprintf(fp_csv_trav, "nodes,inorder_rec_us,inorder_iter_us,preorder_rec_us,preorder_iter_us,postorder_rec_us,postorder_iter_us\n");
    fprintf(fp_csv_tt, "k_atoms,num_rows,time_us,nodes,time_per_row_ns\n");
    fprintf(fp_csv_height, "nodes,type,measured_height,time_rec_us,time_iter_us\n");
    fprintf(fp_csv_scaling, "num_clauses,formula_len,nodes,height,infix_to_prefix_us,prefix_to_tree_us\n");

    fprintf(fp_json, "{\n  \"timestamp\": %ld,\n", (long)time(NULL));

    /* =========================================================================
       1. BENCHMARK: Traversals across varying tree sizes N (Tasks 1, 2, 3)
       ========================================================================= */
    printf("[1/4] Benchmarking Traversals and Conversions (Tasks 1, 2, 3)...\n");
    fprintf(fp_json, "  \"traversal_benchmarks\": [\n");

    int clause_steps[] = { 1, 2, 5, 10, 20, 35, 50, 75, 100, 150 };
    int n_steps = sizeof(clause_steps) / sizeof(clause_steps[0]);

    for (int s = 0; s < n_steps; s++) {
        int c = clause_steps[s];
        /* Synthesize formula of size c */
        g_infix[0] = '\0';
        strcpy(g_infix, "((x1 + x2) + (~x3))");
        for (int i = 1; i < c; i++) {
            char temp[MAX_BUFFER];
            snprintf(temp, sizeof(temp), "(%s * ((x%d + x%d) + (~x%d)))", g_infix, i + 1, i + 2, i + 3);
            strncpy(g_infix, temp, sizeof(g_infix) - 1);
        }

        Node *root = build_parse_tree(g_infix);
        int node_count = count_nodes(root);
        int h = height_rec(root);

        /* Measure Task 1: Infix to Prefix */
        double t0 = get_time_us();
        int iters_conv = 200;
        for (int it = 0; it < iters_conv; it++) {
            infix_to_prefix_bench(g_infix, g_prefix_buf);
        }
        double t1 = get_time_us();
        double time_i2p = (t1 - t0) / iters_conv;

        /* Measure Task 2: Prefix to Tree */
        t0 = get_time_us();
        for (int it = 0; it < iters_conv; it++) {
            Node *tr = build_tree_from_prefix(g_prefix_buf);
            free_tree(tr);
        }
        t1 = get_time_us();
        double time_p2t = (t1 - t0) / iters_conv;

        fprintf(fp_csv_scaling, "%d,%zu,%d,%d,%.3f,%.3f\n",
                c, strlen(g_infix), node_count, h, time_i2p, time_p2t);

        /* Measure 6 Traversals (Task 3) */
        int iters = (node_count > 300) ? 500 : 2000;
        
        /* In-order Rec */
        t0 = get_time_us();
        for (int i = 0; i < iters; i++) bench_inorder_rec(root);
        t1 = get_time_us();
        double t_in_rec = (t1 - t0) / iters;

        /* In-order Iter */
        t0 = get_time_us();
        for (int i = 0; i < iters; i++) bench_inorder_iter(root);
        t1 = get_time_us();
        double t_in_iter = (t1 - t0) / iters;

        /* Pre-order Rec */
        t0 = get_time_us();
        for (int i = 0; i < iters; i++) bench_preorder_rec(root);
        t1 = get_time_us();
        double t_pre_rec = (t1 - t0) / iters;

        /* Pre-order Iter */
        t0 = get_time_us();
        for (int i = 0; i < iters; i++) bench_preorder_iter(root);
        t1 = get_time_us();
        double t_pre_iter = (t1 - t0) / iters;

        /* Post-order Rec */
        t0 = get_time_us();
        for (int i = 0; i < iters; i++) bench_postorder_rec(root);
        t1 = get_time_us();
        double t_post_rec = (t1 - t0) / iters;

        /* Post-order Iter */
        t0 = get_time_us();
        for (int i = 0; i < iters; i++) bench_postorder_iter(root);
        t1 = get_time_us();
        double t_post_iter = (t1 - t0) / iters;

        fprintf(fp_csv_trav, "%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                node_count, t_in_rec, t_in_iter, t_pre_rec, t_pre_iter, t_post_rec, t_post_iter);

        fprintf(fp_json, "    {\"nodes\": %d, \"height\": %d, \"in_rec\": %.4f, \"in_iter\": %.4f, "
                         "\"pre_rec\": %.4f, \"pre_iter\": %.4f, \"post_rec\": %.4f, \"post_iter\": %.4f}%s\n",
                node_count, h, t_in_rec, t_in_iter, t_pre_rec, t_pre_iter, t_post_rec, t_post_iter,
                (s == n_steps - 1) ? "" : ",");

        free_tree(root);
    }
    fprintf(fp_json, "  ],\n");

    /* =========================================================================
       2. BENCHMARK: Height Recursive vs. Iterative (Task 4)
       ========================================================================= */
    printf("[2/4] Benchmarking Height: Recursive vs Iterative (Task 4)...\n");
    fprintf(fp_json, "  \"height_benchmarks\": [\n");

    int leaf_counts[] = { 2, 4, 8, 16, 32, 64, 128, 256 };
    int n_leaf_steps = sizeof(leaf_counts) / sizeof(leaf_counts[0]);

    for (int l = 0; l < n_leaf_steps; l++) {
        int n_leaves = leaf_counts[l];

        /* Build balanced tree */
        g_balanced[0] = '\0';
        char atoms[256][16];
        for (int i = 0; i < n_leaves; i++) snprintf(atoms[i], sizeof(atoms[i]), "a%d", i);

        for (int i = 0; i < n_leaves; i++) strcpy(g_temp_pool[i], atoms[i]);
        int cur_cnt = n_leaves;
        while (cur_cnt > 1) {
            int new_cnt = 0;
            for (int i = 0; i < cur_cnt; i += 2) {
                if (i + 1 < cur_cnt) {
                    snprintf(g_temp_pool[new_cnt], sizeof(g_temp_pool[new_cnt]), "(%s + %s)", g_temp_pool[i], g_temp_pool[i+1]);
                } else {
                    strcpy(g_temp_pool[new_cnt], g_temp_pool[i]);
                }
                new_cnt++;
            }
            cur_cnt = new_cnt;
        }
        strcpy(g_balanced, g_temp_pool[0]);

        Node *r_bal = build_parse_tree(g_balanced);
        int nodes_bal = count_nodes(r_bal);
        int h_bal = height_rec(r_bal);

        double t0 = get_time_us();
        int iters_h = 5000;
        for (int i = 0; i < iters_h; i++) height_rec(r_bal);
        double t1 = get_time_us();
        double t_h_rec = (t1 - t0) / iters_h;

        t0 = get_time_us();
        for (int i = 0; i < iters_h; i++) height_iter(r_bal);
        t1 = get_time_us();
        double t_h_iter = (t1 - t0) / iters_h;

        fprintf(fp_csv_height, "%d,balanced,%d,%.4f,%.4f\n", nodes_bal, h_bal, t_h_rec, t_h_iter);

        /* Build skewed tree: (a1 + (a2 + (a3 + ...))) */
        strcpy(g_skewed, atoms[n_leaves - 1]);
        for (int i = n_leaves - 2; i >= 0; i--) {
            char tmp[MAX_BUFFER];
            snprintf(tmp, sizeof(tmp), "(%s + %s)", atoms[i], g_skewed);
            strncpy(g_skewed, tmp, sizeof(g_skewed) - 1);
        }

        Node *r_skew = build_parse_tree(g_skewed);
        int nodes_skew = count_nodes(r_skew);
        int h_skew = height_rec(r_skew);

        t0 = get_time_us();
        for (int i = 0; i < iters_h; i++) height_rec(r_skew);
        t1 = get_time_us();
        double t_h_rec_sk = (t1 - t0) / iters_h;

        t0 = get_time_us();
        for (int i = 0; i < iters_h; i++) height_iter(r_skew);
        t1 = get_time_us();
        double t_h_iter_sk = (t1 - t0) / iters_h;

        fprintf(fp_csv_height, "%d,skewed,%d,%.4f,%.4f\n", nodes_skew, h_skew, t_h_rec_sk, t_h_iter_sk);

        fprintf(fp_json, "    {\"leaves\": %d, \"balanced_nodes\": %d, \"balanced_height\": %d, \"bal_rec_us\": %.4f, \"bal_iter_us\": %.4f, "
                         "\"skewed_height\": %d, \"skew_rec_us\": %.4f, \"skew_iter_us\": %.4f}%s\n",
                n_leaves, nodes_bal, h_bal, t_h_rec, t_h_iter, h_skew, t_h_rec_sk, t_h_iter_sk,
                (l == n_leaf_steps - 1) ? "" : ",");

        free_tree(r_bal);
        free_tree(r_skew);
    }
    fprintf(fp_json, "  ],\n");

    /* =========================================================================
       3. BENCHMARK: Truth Table Exponential Scaling O(2^k * N) (Task 5)
       ========================================================================= */
    printf("[3/4] Benchmarking Truth Table: Exponential Scaling (Task 5)...\n");
    fprintf(fp_json, "  \"truth_table_benchmarks\": [\n");

    for (int k = 2; k <= 20; k++) {
        /* Generate formula with k distinct atoms */
        snprintf(g_f_str, sizeof(g_f_str), "(p1 + p2)");
        for (int i = 3; i <= k; i++) {
            char temp[MAX_BUFFER];
            char op = (i % 2 == 0) ? '+' : '*';
            snprintf(temp, sizeof(temp), "(%s %c p%d)", g_f_str, op, i);
            strncpy(g_f_str, temp, sizeof(g_f_str) - 1);
        }

        Node *root = build_parse_tree(g_f_str);
        AtomTable t;
        t.count = 0;
        collect_atoms(root, &t);
        int node_count = count_nodes(root);
        long long rows = 1LL << k;

        int iters_tt = 1;
        if (k <= 8)  iters_tt = 100;
        else if (k <= 12) iters_tt = 20;
        else if (k <= 14) iters_tt = 5;

        double t0 = get_time_us();
        for (int it = 0; it < iters_tt; it++) {
            bench_truth_table(root, &t);
        }
        double t1 = get_time_us();
        double time_us = (t1 - t0) / iters_tt;
        double ns_per_row = (time_us * 1000.0) / (double)rows;

        printf("  k = %2d atoms | %8lld rows | %12.2f us | %.2f ns/row\n",
               k, rows, time_us, ns_per_row);

        fprintf(fp_csv_tt, "%d,%lld,%.3f,%d,%.2f\n", k, rows, time_us, node_count, ns_per_row);

        fprintf(fp_json, "    {\"k\": %d, \"rows\": %lld, \"time_us\": %.3f, \"nodes\": %d, \"ns_per_row\": %.2f}%s\n",
                k, rows, time_us, node_count, ns_per_row, (k == 20) ? "" : ",");

        free_tree(root);
    }
    fprintf(fp_json, "  ],\n");

    /* =========================================================================
       4. Memory Analysis Statistics
       ========================================================================= */
    printf("[4/4] Recording Memory Architecture Statistics...\n");
    fprintf(fp_json, "  \"memory_stats\": {\n");
    fprintf(fp_json, "    \"sizeof_node_bytes\": %zu,\n", sizeof(Node));
    fprintf(fp_json, "    \"sizeof_ptr_bytes\": %zu,\n", sizeof(void *));
    fprintf(fp_json, "    \"sizeof_label_bytes\": 32,\n");
    fprintf(fp_json, "    \"sizeof_callframe_approx_bytes\": 64,\n");
    fprintf(fp_json, "    \"max_atoms_supported\": %d\n", MAX_ATOMS);
    fprintf(fp_json, "  }\n");
    fprintf(fp_json, "}\n");

    fclose(fp_json);
    fclose(fp_csv_trav);
    fclose(fp_csv_tt);
    fclose(fp_csv_height);
    fclose(fp_csv_scaling);

    printf("Benchmark Engine successfully finished. Datasets written to Part-A/analysis/data/.\n");
    return 0;
}
