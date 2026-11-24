// prompt: apply one rule to every left row of a scan: copy, chain join, or negation against the first stop rows.
def Stigi.Datalog.applyRule {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), slots: Array[Stigi.Datalog.Slot](rowCap), spec: Stigi.Datalog.RuleSpec, scan: Stigi.Datalog.Scan, remaining: U64) <below(0, rowCap), decreases(remaining)>: Stigi.Datalog.Derive =
  val Scan(idx, stop, total) = scan
  if remaining < 1 || stop <= idx || rowCap <= idx then Derived(total) else
    val RuleSpec(headRel, leftRel, rightRel, shape) = spec
    val left: Stigi.Datalog.TupleRow = rows(idx)
    val TupleRow(rel, colZero, colOne, colTwo) = left
    val step: Stigi.Datalog.Derive =
      if rel == leftRel then
        shape match
          case JoinCopy => Stigi.Datalog.emitRow(rowCap, rows, slots, total, TupleRow(headRel, colZero, colOne, colTwo))
          case JoinChain => Stigi.Datalog.joinRight(rowCap, rows, slots, spec, left, Scan(0, stop, total), rowCap)
          case JoinAntiMatch =>
            val cleared: Stigi.Datalog.TupleRow = TupleRow(rightRel, colZero, colOne, colTwo)
            val known: U64 = if stop <= rowCap then stop else rowCap
            if Stigi.Datalog.probeSlots(rowCap, rows, known, slots, cleared, Stigi.Datalog.tupleKey(cleared, rowCap), rowCap) is FoundAt(_) then Derived(total) else Stigi.Datalog.emitRow(rowCap, rows, slots, total, TupleRow(headRel, colZero, colOne, colTwo))
      else Derived(total)
    step match
      case Derived(next) => Stigi.Datalog.applyRule(rowCap, rows, slots, spec, Scan(idx + 1, stop, next), remaining - 1)
      case DeriveFull => DeriveFull
