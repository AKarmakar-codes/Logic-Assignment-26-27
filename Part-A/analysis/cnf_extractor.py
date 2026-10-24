#!/usr/bin/env python3
"""
CNF Extractor & Test Formula Generator for Logic Assignment Benchmark.
Scans SAT 2002 Competition benchmark suites:
- sat-2002-beta/submitted/pehoushek/twentyvars (Exact 20-atom competition benchmark)
- sat-2002-beta/generated (Uniform random 3-SAT instances)
- sat-2002-beta/submitted (Industrial and challenge instances)

Produces:
- Benchmark metadata across the suite
- Extracted parenthesized infix formulas of controlled sizes for Tasks 1-5
"""

import os
import glob
import json
import sys

def parse_dimacs_cnf(filepath, max_clauses=None):
    """Parse a DIMACS .cnf file into a list of clauses (lists of integer literals)."""
    clauses = []
    num_vars = 0
    num_clauses = 0
    
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('c'):
                continue
            if line.startswith('p cnf'):
                parts = line.split()
                if len(parts) >= 4:
                    num_vars = int(parts[2])
                    num_clauses = int(parts[3])
                continue
            
            # Read clause literals
            tokens = line.split()
            lits = []
            for t in tokens:
                val = int(t)
                if val == 0:
                    if lits:
                        clauses.append(lits)
                        lits = []
                        if max_clauses and len(clauses) >= max_clauses:
                            return clauses, num_vars, num_clauses
                else:
                    lits.append(val)
            if lits and (not max_clauses or len(clauses) < max_clauses):
                clauses.append(lits)
                
    return clauses, num_vars, num_clauses

def clause_to_infix(clause, var_map=None):
    """Convert a single clause [l1, l2, ...] into a parenthesized infix expression."""
    formatted = []
    for lit in clause:
        var_num = abs(lit)
        name = var_map.get(var_num, f"x{var_num}") if var_map else f"x{var_num}"
        if lit < 0:
            formatted.append(f"(~{name})")
        else:
            formatted.append(name)
            
    if not formatted:
        return "x0"
    if len(formatted) == 1:
        return formatted[0]
        
    res = formatted[0]
    for lit_str in formatted[1:]:
        res = f"({res} + {lit_str})"
    return res

def clauses_to_cnf_infix(clauses, var_map=None, balanced=True):
    """Combine clauses with AND '*' into a parenthesized formula."""
    if not clauses:
        return "(x1 + x2)"
    clause_strs = [clause_to_infix(c, var_map) for c in clauses]
    
    if balanced:
        def combine_balanced(lst):
            if len(lst) == 1:
                return lst[0]
            mid = len(lst) // 2
            left = combine_balanced(lst[:mid])
            right = combine_balanced(lst[mid:])
            return f"({left} * {right})"
        return combine_balanced(clause_strs)
    else:
        res = clause_strs[0]
        for c in clause_strs[1:]:
            res = f"({res} * {c})"
        return res

def generate_controlled_atom_formula(k):
    """
    Generate a formula with exactly k distinct atoms: a1, a2, ..., ak
    Used specifically to benchmark Task 5 Truth Table scaling 2^k.
    """
    atoms = [f"p{i+1}" for i in range(k)]
    if k == 1:
        return atoms[0]
    if k == 2:
        return f"({atoms[0]} + {atoms[1]})"
        
    def build_tree(lst):
        if len(lst) == 1:
            return lst[0]
        if len(lst) == 2:
            return f"({lst[0]} * (~{lst[1]}))"
        mid = len(lst) // 2
        left = build_tree(lst[:mid])
        right = build_tree(lst[mid:])
        op = "+" if len(lst) % 2 == 0 else ">"
        return f"({left} {op} {right})"
        
    return build_tree(atoms)

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    workspace_root = os.path.abspath(os.path.join(script_dir, "../.."))
    sat_root = os.path.join(workspace_root, "sat-2002-beta")
    output_dir = os.path.join(script_dir, "data")
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"Scanning SAT 2002 benchmark suites under {sat_root}...")
    all_cnf_files = glob.glob(os.path.join(sat_root, "**/*.cnf"), recursive=True)
    print(f"Total CNF benchmark files discovered: {len(all_cnf_files)}")
    
    # Analyze benchmark metadata across categories
    categories = {}
    sampled_metadata = []
    
    for fpath in all_cnf_files:
        rel = os.path.relpath(fpath, sat_root)
        cat = rel.split(os.sep)[0] if os.sep in rel else "root"
        categories[cat] = categories.get(cat, 0) + 1
        
    print("Benchmark categories:", categories)
    
    # Sample up to 50 files for suite statistical landscape
    sample_files = all_cnf_files[:60]
    for fpath in sample_files:
        try:
            cl, nv, nc = parse_dimacs_cnf(fpath, max_clauses=2)
            if nv > 0 and nc > 0:
                sampled_metadata.append({
                    "file": os.path.basename(fpath),
                    "path": os.path.relpath(fpath, sat_root),
                    "num_variables": nv,
                    "num_clauses": nc,
                    "clause_var_ratio": round(nc / nv, 2)
                })
        except Exception:
            pass

    # Extract genuine 20-var formulas from pehoushek/twentyvars
    twentyvar_files = glob.glob(os.path.join(sat_root, "submitted/pehoushek/twentyvars/*.cnf"))
    twentyvar_sample = None
    if twentyvar_files:
        cl, nv, nc = parse_dimacs_cnf(twentyvar_files[0], max_clauses=10)
        twentyvar_sample = {
            "source_file": os.path.basename(twentyvar_files[0]),
            "num_variables": nv,
            "extracted_clauses": len(cl),
            "infix": clauses_to_cnf_infix(cl, balanced=True)
        }
        print(f"Loaded 20-var competition benchmark sample: {twentyvar_sample['source_file']}")
        
    # Generate formulas for Task 5 Truth Table (k from 2 to 20)
    truth_table_inputs = []
    for k in range(2, 21):
        formula = generate_controlled_atom_formula(k)
        truth_table_inputs.append({
            "k_atoms": k,
            "formula": formula,
            "num_rows": 2 ** k
        })
        
    # Save dataset
    dataset = {
        "total_cnf_files": len(all_cnf_files),
        "categories": categories,
        "suite_samples": sampled_metadata,
        "twentyvar_sample": twentyvar_sample,
        "truth_table_inputs": truth_table_inputs
    }
    
    out_json = os.path.join(output_dir, "benchmark_inputs.json")
    with open(out_json, "w", encoding="utf-8") as f:
        json.dump(dataset, f, indent=2)
    print(f"Benchmark inputs updated successfully -> {out_json}")

if __name__ == "__main__":
    main()
