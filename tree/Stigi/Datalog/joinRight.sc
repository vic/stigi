// prompt: join one left row with every right row of a chain rule in a scan, deriving the head rows.
def Stigi.Datalog.joinRight {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), slots: Array[Stigi.Datalog.Slot](rowCap), spec: Stigi.Datalog.RuleSpec, left: Stigi.Datalog.TupleRow, scan: Stigi.Datalog.Scan, remaining: U64) <below(0, rowCap), decreases(remaining)>: Stigi.Datalog.Derive =
  val Scan(idx, stop, total) = scan
  if remaining < 1 || stop <= idx || rowCap <= idx then Derived(total) else
    val RuleSpec(headRel, _, rightRel, _) = spec
    val TupleRow(_, leftZero, leftOne, leftTwo) = left
    val TupleRow(rel, colZero, colOne, colTwo) = rows(idx)
    val step: Stigi.Datalog.Derive = if rel == rightRel && leftOne == colZero && leftTwo == colTwo then Stigi.Datalog.emitRow(rowCap, rows, slots, total, TupleRow(headRel, leftZero, colOne, leftTwo)) else Derived(total)
    step match
      case Derived(next) => Stigi.Datalog.joinRight(rowCap, rows, slots, spec, left, Scan(idx + 1, stop, next), remaining - 1)
      case DeriveFull => DeriveFull
