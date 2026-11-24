// prompt: run the stratified Datalog door rules over extracted facts using caller-supplied slot and link tables.
def Stigi.Door.runEngineCheck {region: Stigi.Region, Stigi.Mut} (factCap: Stigi.NodeCap, facts: Array[Stigi.Extract.FactRow](factCap), factCount: U64, nodeCount: U64, rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.Store.Tuple](rowCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), tupleSlotCap: U64, tupleSlots: Array[Stigi.Datalog.Store.Slot](tupleSlotCap)) <lessOrEqual(factCount, factCap), lessOrEqual(factCap, rowCap), below(0, rowCap), below(0, tupleSlotCap)>: Stigi.Door.Verdict =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val padTerm: Stigi.Datalog.Check.Term = ConstSym(0)
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val varTwo: Stigi.Datalog.Check.Term = VarTerm(2)
  val varThree: Stigi.Datalog.Check.Term = VarTerm(3)
  val emptyAtom: Stigi.Datalog.Check.Atom = AtomRef(0, 3, padTerm, padTerm, padTerm, padTerm, padTerm, padTerm, padTerm)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  val initFalse: Bool = false
  val zeroWord: U64 = 0
  val threeWord: U64 = 3
  val Filled(relHead) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(12, StoreVacant) else
    case FillFailed => DoorOutOfMemory
  val Filled(relArities) = Array.tryFill[U64]{region}(12, threeWord) else
    case FillFailed => DoorOutOfMemory
  val Filled(rules) = Array.tryFill[Stigi.Datalog.Check.RuleDef]{region}(4, emptyRule) else
    case FillFailed => DoorOutOfMemory
  val Filled(lits) = Array.tryFill[Stigi.Datalog.Check.Lit]{region}(8, emptyLit) else
    case FillFailed => DoorOutOfMemory
  val Filled(bound) = Array.tryFill[Bool]{region}(4, initFalse) else
    case FillFailed => DoorOutOfMemory
  val Filled(strata) = Array.tryFill[U64]{region}(12, zeroWord) else
    case FillFailed => DoorOutOfMemory
  val Filled(varBound) = Array.tryFill[Bool]{region}(4, initFalse) else
    case FillFailed => DoorOutOfMemory
  val Filled(varVals) = Array.tryFill[Stigi.Datalog.Store.Cell]{region}(4, padCell) else
    case FillFailed => DoorOutOfMemory
  val Filled(ranks) = Array.tryFill[U64]{region}(4, zeroWord) else
    case FillFailed => DoorOutOfMemory
  val Filled(iterSlot) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(8, StoreVacant) else
    case FillFailed => DoorOutOfMemory
  val Filled(iterMask) = Array.tryFill[U64]{region}(8, zeroWord) else
    case FillFailed => DoorOutOfMemory
  var idx: U64 = 0
  var curRows: U64 = 0
  var fullFlag: Bool = false
  while idx < factCount && !fullFlag do
    @<lessOrEqual(idx, factCount), lessOrEqual(curRows, rowCap)>
    val nextIdx: U64 = idx + 1
    val FactRow(factKind, colZero, colOne, colTwo) = facts(idx)
    val relTag: U64 = factKind match
      case FactBelow => 1
      case FactLessEqual => 2
      case FactIndexOwed => 3
      case FactCall => 4
      case FactAlloc => 5
      case FactFreeEdge => 6
      case FactStackObject => 7
      case FactGivenCheck => 8
      case FactCapState => 9
      case FactTaint => 10
    val rowItem: Stigi.Datalog.Store.Tuple = StoreRow(relTag, 3, SymCell(colZero), SymCell(colOne), SymCell(colTwo), padCell, padCell, padCell, padCell)
    Stigi.Datalog.Store.insertTuple(12, rowCap, tupleSlotCap, rows, tupleSlots, relHead, nextInRel, rowItem, curRows) match
      case TupleAdded(_, nextRows) =>
        if nextRows <= rowCap then curRows = nextRows else fullFlag = true
      case TupleDuplicate(_) => ()
      case TupleTableFull => fullFlag = true
    idx = nextIdx
  if fullFlag then DoorDatalogRejected(1) else
    lits(0) := PosLit(AtomRef(1, 3, varZero, varOne, varTwo, padTerm, padTerm, padTerm, padTerm))
    lits(1) := PosLit(AtomRef(2, 3, varOne, varThree, varTwo, padTerm, padTerm, padTerm, padTerm))
    lits(2) := PosLit(AtomRef(1, 3, varZero, varOne, varTwo, padTerm, padTerm, padTerm, padTerm))
    lits(3) := PosLit(AtomRef(3, 3, varZero, varOne, varTwo, padTerm, padTerm, padTerm, padTerm))
    lits(4) := NegLit(AtomRef(1, 3, varZero, varOne, varTwo, padTerm, padTerm, padTerm, padTerm))
    rules(0) := RuleEntry(AtomRef(1, 3, varZero, varThree, varTwo, padTerm, padTerm, padTerm, padTerm), 0, 2)
    rules(1) := RuleEntry(AtomRef(2, 3, varZero, varOne, varTwo, padTerm, padTerm, padTerm, padTerm), 2, 3)
    rules(2) := RuleEntry(AtomRef(11, 3, varZero, varOne, varTwo, padTerm, padTerm, padTerm, padTerm), 3, 5)
    Stigi.Datalog.Engine.runProgram(12, rowCap, tupleSlotCap, 4, 8, 4, 4, relArities, 12, rules, 3, lits, 5, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, curRows, rowCap) match
      case EvalDone(totalRows, _) =>
        val rejCount: U64 = Stigi.Datalog.Engine.countRelationRows(12, rowCap, relHead, nextInRel, 11)
        if rejCount == 0 then DoorAccepted(nodeCount, factCount, totalRows) else DoorDatalogRejected(rejCount)
      case EvalRefused(_) => DoorDatalogRejected(1)
      case EvalBudgetExceeded(_) => DoorDatalogRejected(1)
      case EvalTableFull => DoorDatalogRejected(1)
      case EvalArithError => DoorDatalogRejected(1)
