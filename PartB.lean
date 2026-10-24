/-
================================================================================
  CS F214: Logic in Computer Science
  BITS Pilani, Hyderabad Campus
  Assignment 1 — Part B: Formal Proofs in LEAN 4
  Textbook: "Logic in Computer Science: Modelling and Reasoning about Systems"
            by Michael Huth and Mark Ryan (Cambridge University Press)
================================================================================

  TASKS COVERED:
  • Task 1: Complete formal proofs for Examples 1.13 – 1.18 of the textbook [5 Marks]
  • Task 2: Comprehensive demo & viva preparation sequents with step-by-step
            natural deduction derivations
-/

open Classical

-- ==============================================================================
-- SECTION 0: Natural Deduction to Lean 4 Quick Reference
-- ==============================================================================
/-
  Rule Name                  Natural Deduction       Lean 4 Tactic / Expression
  -----------------------------------------------------------------------------
  Implication Intro (→i)     Assume p, derive q      `intro hp`
  Implication Elim  (→e)     p → q, p ⊢ q            `h_imp hp` or `apply h_imp`
  Conjunction Intro (∧i)     p, q ⊢ p ∧ q            `And.intro hp hq` or `⟨hp, hq⟩`
  Conjunction Elim1 (∧e1)    p ∧ q ⊢ p               `hpq.left` or `hpq.1`
  Conjunction Elim2 (∧e2)    p ∧ q ⊢ q               `hpq.right` or `hpq.2`
  Disjunction Intro1(∨i1)    p ⊢ p ∨ q               `Or.inl hp`
  Disjunction Intro2(∨i2)    q ⊢ p ∨ q               `Or.inr hq`
  Disjunction Elim  (∨e)     p ∨ q, p⊢r, q⊢r ⊢ r     `cases hpq with | inl hp => ... | inr hq => ...`
  Negation Intro    (¬i)     Assume p, derive ⊥      `intro hp; ... exact hbot`
  Negation Elim     (¬e)     p, ¬p ⊢ ⊥               `hnp hp`
  Bottom Elim       (⊥e)     ⊥ ⊢ p                   `False.elim hbot`
  Double Neg Elim   (¬¬e)    ¬¬p ⊢ p                 `apply Classical.byContradiction; intro hnp; ...`
  Law of Excl. Mid. (LEM)    ⊢ p ∨ ¬p                `Classical.em p`
-/

-- Handout Example: Conjunction Introduction
-- Sequent: P, Q ⊢ P ∧ Q
theorem and_intro_handout (P Q : Prop) : P → Q → P ∧ Q := by
  intro hP
  intro hQ
  exact And.intro hP hQ


-- ==============================================================================
-- SECTION 1: TASK 1 — Examples 1.13 to 1.18 from Textbook
-- ==============================================================================

--------------------------------------------------------------------------------
-- EXAMPLE 1.13 (Textbook Section 1.2, p. 13)
-- Sequent: p ∧ q → r ⊢ p → (q → r)
--
-- Natural Deduction Proof:
--   1. p ∧ q → r             premise
--   2.   p                   assumption
--   3.     q                 assumption
--   4.     p ∧ q             ∧i 2, 3
--   5.     r                 →e 1, 4
--   6.   q → r               →i 3-5
--   7. p → (q → r)           →i 2-6
--------------------------------------------------------------------------------

-- Style 1: Premise as implication (as introduced in the assignment handout)
theorem example_1_13 (p q r : Prop) : (p ∧ q → r) → (p → (q → r)) := by
  intro h
  intro hp
  intro hq
  have hpq : p ∧ q := And.intro hp hq
  exact h hpq

-- Style 2: Premise as hypothesis argument in signature
theorem example_1_13_hyp (p q r : Prop) (h : p ∧ q → r) : p → (q → r) := by
  intro hp
  intro hq
  have hpq : p ∧ q := And.intro hp hq
  exact h hpq


--------------------------------------------------------------------------------
-- EXAMPLE 1.14 (Textbook Section 1.2, p. 13)
-- Sequent: p → (q → r) ⊢ p ∧ q → r
--
-- Natural Deduction Proof:
--   1. p → (q → r)           premise
--   2.   p ∧ q               assumption
--   3.   p                   ∧e1 2
--   4.   q                   ∧e2 2
--   5.   q → r               →e 1, 3
--   6.   r                   →e 5, 4
--   7. p ∧ q → r             →i 2-6
--------------------------------------------------------------------------------

-- Style 1: Premise as implication
theorem example_1_14 (p q r : Prop) : (p → (q → r)) → (p ∧ q → r) := by
  intro h
  intro hpq
  have hp : p := hpq.left
  have hq : q := hpq.right
  have hqr : q → r := h hp
  exact hqr hq

-- Style 2: Premise as hypothesis argument in signature
theorem example_1_14_hyp (p q r : Prop) (h : p → (q → r)) : p ∧ q → r := by
  intro hpq
  have hp : p := hpq.left
  have hq : q := hpq.right
  have hqr : q → r := h hp
  exact hqr hq


--------------------------------------------------------------------------------
-- EXAMPLE 1.15 (Textbook Section 1.2, p. 14)
-- Sequent: p → q ⊢ (p ∧ r) → (q ∧ r)
--
-- Natural Deduction Proof:
--   1. p → q                 premise
--   2.   p ∧ r               assumption
--   3.   p                   ∧e1 2
--   4.   r                   ∧e2 2
--   5.   q                   →e 1, 3
--   6.   q ∧ r               ∧i 5, 4
--   7. (p ∧ r) → (q ∧ r)     →i 2-6
--------------------------------------------------------------------------------

-- Style 1: Premise as implication
theorem example_1_15 (p q r : Prop) : (p → q) → ((p ∧ r) → (q ∧ r)) := by
  intro h
  intro hpr
  have hp : p := hpr.left
  have hr : r := hpr.right
  have hq : q := h hp
  exact And.intro hq hr

-- Style 2: Premise as hypothesis argument in signature
theorem example_1_15_hyp (p q r : Prop) (h : p → q) : (p ∧ r) → (q ∧ r) := by
  intro hpr
  have hp : p := hpr.left
  have hr : r := hpr.right
  have hq : q := h hp
  exact And.intro hq hr


--------------------------------------------------------------------------------
-- EXAMPLE 1.16 (Textbook Section 1.2, p. 15)
-- Sequent: q → r ⊢ (p ∨ q) → (p ∨ r)
--
-- Natural Deduction Proof:
--   1. q → r                 premise
--   2.   p ∨ q               assumption
--   3.     p                 assumption (case 1)
--   4.     p ∨ r             ∨i1 3
--   5.     q                 assumption (case 2)
--   6.     r                 →e 1, 5
--   7.     p ∨ r             ∨i2 6
--   8.   p ∨ r               ∨e 2, 3-4, 5-7
--   9. (p ∨ q) → (p ∨ r)     →i 2-8
--------------------------------------------------------------------------------

-- Style 1: Premise as implication
theorem example_1_16 (p q r : Prop) : (q → r) → ((p ∨ q) → (p ∨ r)) := by
  intro h
  intro hpq
  cases hpq with
  | inl hp =>
    exact Or.inl hp
  | inr hq =>
    have hr : r := h hq
    exact Or.inr hr

-- Style 2: Premise as hypothesis argument in signature
theorem example_1_16_hyp (p q r : Prop) (h : q → r) : (p ∨ q) → (p ∨ r) := by
  intro hpq
  cases hpq with
  | inl hp =>
    exact Or.inl hp
  | inr hq =>
    have hr : r := h hq
    exact Or.inr hr


--------------------------------------------------------------------------------
-- EXAMPLE 1.17 (Textbook Section 1.2, p. 16)
-- Sequent: (p ∨ q) ∨ r ⊢ p ∨ (q ∨ r)
--
-- Natural Deduction Proof:
--   1. (p ∨ q) ∨ r           premise
--   2.   p ∨ q               assumption (case 1)
--   3.     p                 assumption (sub-case 1a)
--   4.     p ∨ (q ∨ r)       ∨i1 3
--   5.     q                 assumption (sub-case 1b)
--   6.     q ∨ r             ∨i1 5
--   7.     p ∨ (q ∨ r)       ∨i2 6
--   8.     p ∨ (q ∨ r)       ∨e 2, 3-4, 5-7
--   9.   r                   assumption (case 2)
--  10.   q ∨ r               ∨i2 9
--  11.   p ∨ (q ∨ r)         ∨i2 10
--  12. p ∨ (q ∨ r)           ∨e 1, 2-8, 9-11
--------------------------------------------------------------------------------

-- Style 1: Premise as implication
theorem example_1_17 (p q r : Prop) : ((p ∨ q) ∨ r) → (p ∨ (q ∨ r)) := by
  intro h
  cases h with
  | inl hpq =>
    cases hpq with
    | inl hp =>
      exact Or.inl hp
    | inr hq =>
      exact Or.inr (Or.inl hq)
  | inr hr =>
    exact Or.inr (Or.inr hr)

-- Style 2: Premise as hypothesis argument in signature
theorem example_1_17_hyp (p q r : Prop) (h : (p ∨ q) ∨ r) : p ∨ (q ∨ r) := by
  cases h with
  | inl hpq =>
    cases hpq with
    | inl hp =>
      exact Or.inl hp
    | inr hq =>
      exact Or.inr (Or.inl hq)
  | inr hr =>
    exact Or.inr (Or.inr hr)


--------------------------------------------------------------------------------
-- EXAMPLE 1.18 (Textbook Section 1.2, p. 17)
-- Sequent: p ∧ (q ∨ r) ⊢ (p ∧ q) ∨ (p ∧ r)
--
-- Natural Deduction Proof:
--   1. p ∧ (q ∨ r)                   premise
--   2. p                             ∧e1 1
--   3. q ∨ r                         ∧e2 1
--   4.   q                           assumption (case 1)
--   5.   p ∧ q                       ∧i 2, 4
--   6.   (p ∧ q) ∨ (p ∧ r)           ∨i1 5
--   7.   r                           assumption (case 2)
--   8.   p ∧ r                       ∧i 2, 7
--   9.   (p ∧ q) ∨ (p ∧ r)           ∨i2 8
--  10. (p ∧ q) ∨ (p ∧ r)            ∨e 3, 4-6, 7-9
--------------------------------------------------------------------------------

-- Style 1: Premise as implication
theorem example_1_18 (p q r : Prop) : (p ∧ (q ∨ r)) → ((p ∧ q) ∨ (p ∧ r)) := by
  intro h
  have hp : p := h.left
  have hqr : q ∨ r := h.right
  cases hqr with
  | inl hq =>
    have hpq : p ∧ q := And.intro hp hq
    exact Or.inl hpq
  | inr hr =>
    have hpr : p ∧ r := And.intro hp hr
    exact Or.inr hpr
