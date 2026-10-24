/-
================================================================================
  CS F214: Logic in Computer Science  |  BITS Pilani, Hyderabad Campus
  Assignment 1 - Part B: Natural Deduction proofs in Lean 4
  Textbook: Huth & Ryan, "Logic in Computer Science"
================================================================================
  Convention used throughout (same as the assignment example):
    A sequent  p1, p2 |- c  is written in Lean as the implication
        p1 -> p2 -> c
    and the proof begins by `intro`-ing each premise (->i), exactly like
        theorem and_intro (P Q : Prop) : P -> Q -> P /\ Q

  Natural deduction  ->  Lean formulas
    ->i   intro h                  ->e   h hp          (modus ponens)
    /\i   And.intro a b  or <a, b> /\e1  h.left        /\e2  h.right
    \/i1  Or.inl a                 \/i2  Or.inr b
    \/e   cases h with | inl x => .. | inr y => ..
    ~i    intro hp  (goal False)   ~e    hn hp         (gives False)
    Fe    False.elim h
    PBC   Classical.byContradiction (fun hn => ..)
    LEM   Classical.em p
================================================================================
-/

-- Assignment example:  P, Q |- P /\ Q
theorem and_intro_handout (P Q : Prop) : P → Q → P ∧ Q := by
  intro hP
  intro hQ
  exact And.intro hP hQ

/-
================================================================================
  TASK 1: Examples 1.13 - 1.18
================================================================================
-/

-- Example 1.13 :  p /\ q -> r  |-  p -> (q -> r)
theorem example_1_13 (p q r : Prop) : (p ∧ q → r) → (p → (q → r)) := by
  intro h                        -- premise p /\ q -> r
  intro hp                       -- assume p
  intro hq                       -- assume q
  have hpq : p ∧ q := And.intro hp hq   -- /\i
  exact h hpq                    -- ->e

-- Example 1.14 :  p -> (q -> r)  |-  p /\ q -> r
theorem example_1_14 (p q r : Prop) : (p → (q → r)) → (p ∧ q → r) := by
  intro h                        -- premise p -> (q -> r)
  intro hpq                      -- assume p /\ q
  have hp : p := hpq.left        -- /\e1
  have hq : q := hpq.right       -- /\e2
  have hqr : q → r := h hp       -- ->e
  exact hqr hq                   -- ->e

-- Example 1.15 :  p -> q  |-  (p /\ r) -> (q /\ r)
theorem example_1_15 (p q r : Prop) : (p → q) → ((p ∧ r) → (q ∧ r)) := by
  intro h                        -- premise p -> q
  intro hpr                      -- assume p /\ r
  have hp : p := hpr.left        -- /\e1
  have hr : r := hpr.right       -- /\e2
  have hq : q := h hp            -- ->e
  exact And.intro hq hr          -- /\i

-- Example 1.16 :  q -> r  |-  (p \/ q) -> (p \/ r)
theorem example_1_16 (p q r : Prop) : (q → r) → ((p ∨ q) → (p ∨ r)) := by
  intro h                        -- premise q -> r
  intro hpq                      -- assume p \/ q
  cases hpq with                 -- \/e
  | inl hp => exact Or.inl hp    --   case p : \/i1
  | inr hq =>                    --   case q
    have hr : r := h hq          --   ->e
    exact Or.inr hr              --   \/i2

-- Example 1.17 :  (p \/ q) \/ r  |-  p \/ (q \/ r)
theorem example_1_17 (p q r : Prop) : ((p ∨ q) ∨ r) → (p ∨ (q ∨ r)) := by
  intro h                        -- premise (p \/ q) \/ r
  cases h with                   -- \/e on (p \/ q) \/ r
  | inl hpq =>
    cases hpq with               --   \/e on p \/ q
    | inl hp => exact Or.inl hp                 -- \/i1
    | inr hq => exact Or.inr (Or.inl hq)        -- \/i1 then \/i2
  | inr hr => exact Or.inr (Or.inr hr)          -- \/i2 twice

-- Example 1.18 :  p /\ (q \/ r)  |-  (p /\ q) \/ (p /\ r)
theorem example_1_18 (p q r : Prop) : (p ∧ (q ∨ r)) → ((p ∧ q) ∨ (p ∧ r)) := by
  intro h                        -- premise p /\ (q \/ r)
  have hp : p := h.left          -- /\e1
  have hqr : q ∨ r := h.right    -- /\e2
  cases hqr with                 -- \/e
  | inl hq => exact Or.inl (And.intro hp hq)    -- /\i then \/i1
  | inr hr => exact Or.inr (And.intro hp hr)    -- /\i then \/i2
