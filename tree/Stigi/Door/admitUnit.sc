// prompt: admit one standalone unit by allocating bounded scratch arenas and invoking Stigi.Door.verifyUnit.
def Stigi.Door.admitUnit {region: Stigi.Region, Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64) <lessOrEqual(srcLen, srcCap)>: Stigi.Door.Verdict =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val Filled(toks) = Array.tryFill[Stigi.Lex.Token]{region}(256, Token(TokEnd, 0, 0, 0, 0)) else
    case FillFailed => DoorOutOfMemory
  val Filled(nodes) = Array.tryFill[Stigi.Ast.Node]{region}(256, Node(ExprUnit, 0, 0, 0, 0, 0)) else
    case FillFailed => DoorOutOfMemory
  val Filled(facts) = Array.tryFill[Stigi.Extract.FactRow]{region}(256, FactRow(FactCall, 0, 0, 0)) else
    case FillFailed => DoorOutOfMemory
  val Filled(rows) = Array.tryFill[Stigi.Datalog.Store.Tuple]{region}(512, StoreRow(0, 0, padCell, padCell, padCell, padCell, padCell, padCell, padCell)) else
    case FillFailed => DoorOutOfMemory
  val Filled(nextInRel) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(512, StoreVacant) else
    case FillFailed => DoorOutOfMemory
  val Filled(smallSlots) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(512, StoreVacant) else
    case FillFailed => DoorOutOfMemory
  Stigi.Door.verifyUnit{region}(srcCap, src, srcLen, 256, toks, 256, nodes, 256, facts, 512, rows, nextInRel, smallSlots)
