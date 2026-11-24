// prompt: run the positive rules over each new delta window until no row is new or the rounds run out.
def Stigi.Datalog.fixpoint {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), slots: Array[Stigi.Datalog.Slot](rowCap), ruleCap: Stigi.UnitCap, rules: Array[Stigi.Datalog.RuleSpec](ruleCap), ruleCount: U64, window: Stigi.Datalog.Scan, remaining: U64) <below(0, rowCap), lessOrEqual(ruleCount, ruleCap), decreases(remaining)>: Stigi.Datalog.Derive =
  val Scan(start, stop, total) = window
  if remaining < 1 || stop <= start then Derived(total) else
    Stigi.Datalog.applyRules(rowCap, rows, slots, ruleCap, rules, ruleCount, Positive, RulePass(0, start, stop, total), ruleCount) match
      case Derived(next) => Stigi.Datalog.fixpoint(rowCap, rows, slots, ruleCap, rules, ruleCount, Scan(stop, next, next), remaining - 1)
      case DeriveFull => DeriveFull
