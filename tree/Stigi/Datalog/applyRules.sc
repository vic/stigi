// prompt: apply the rules of one stratum from a rule index to one delta window of rows.
def Stigi.Datalog.applyRules {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), slots: Array[Stigi.Datalog.Slot](rowCap), ruleCap: Stigi.UnitCap, rules: Array[Stigi.Datalog.RuleSpec](ruleCap), ruleCount: U64, phase: Stigi.Datalog.Phase, pass: Stigi.Datalog.RulePass, remaining: U64) <below(0, rowCap), lessOrEqual(ruleCount, ruleCap), decreases(remaining)>: Stigi.Datalog.Derive =
  val RulePass(ruleIdx, start, stop, total) = pass
  if remaining < 1 || ruleCount <= ruleIdx then Derived(total) else
    val spec: Stigi.Datalog.RuleSpec = rules(ruleIdx)
    val RuleSpec(_, _, _, shape) = spec
    val step: Stigi.Datalog.Derive = if shape is JoinAntiMatch == (phase is Negative) then Stigi.Datalog.applyRule(rowCap, rows, slots, spec, Scan(start, stop, total), rowCap) else Derived(total)
    step match
      case Derived(next) => Stigi.Datalog.applyRules(rowCap, rows, slots, ruleCap, rules, ruleCount, phase, RulePass(ruleIdx + 1, start, stop, next), remaining - 1)
      case DeriveFull => DeriveFull
