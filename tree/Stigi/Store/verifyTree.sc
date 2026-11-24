// prompt: load every unit listed in <dir>/index and verify each through the full self-hosted door.
def Stigi.Store.verifyTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.RunOutcome =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val Filled(pathBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(indexBuf) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(unitBuf) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(toks) = Array.tryFill[Stigi.Lex.Token]{region}(16384, Token(TokEnd, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(nodes) = Array.tryFill[Stigi.Ast.Node]{region}(16384, Node(ExprUnit, 0, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(facts) = Array.tryFill[Stigi.Extract.FactRow]{region}(16384, FactRow(FactCall, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(rows) = Array.tryFill[Stigi.Datalog.Store.Tuple]{region}(16384, StoreRow(0, 0, padCell, padCell, padCell, padCell, padCell, padCell, padCell)) else
    case FillFailed => RunOutOfMemory
  val Filled(nextInRel) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(16384, StoreVacant) else
    case FillFailed => RunOutOfMemory
  val Filled(smallSlots) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(512, StoreVacant) else
    case FillFailed => RunOutOfMemory
  val Found(idxPathLen) = Stigi.Store.buildIndexPath(dirCap, dirBuf, dirStart, dirEnd, pathBuf) else
    case Missing => RunFileError
  if idxPathLen >= 4096 then RunFileError else
    val LoadedBytes(indexLen) = Stigi.Posix.readFileBytes{region}(idxPathLen, pathBuf, 65536, indexBuf, 0) else
      case LoadFailed => RunFileError
      case LoadOverflow => RunFileError
    if indexLen > 65536 then RunFileError else
      Stigi.Store.verifyLines{region}(dirCap, dirBuf, dirStart, dirEnd, Scratch(pathBuf, indexBuf, unitBuf, toks, nodes, facts, rows, nextInRel, smallSlots), indexLen, IndexWalk(0, 0), indexLen + 1)
