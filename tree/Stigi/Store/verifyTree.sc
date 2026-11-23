// prompt: load every unit listed in <dir>/index and verify each through the full self-hosted door.
def Stigi.Store.verifyTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.RunOutcome =
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
  val Filled(rows) = Array.tryFill[Stigi.Datalog.TupleRow]{region}(16384, TupleRow(0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(rules) = Array.tryFill[Stigi.Datalog.RuleSpec]{region}(4, RuleSpec(0, 0, 0, JoinCopy)) else
    case FillFailed => RunOutOfMemory
  val Found(idxPathLen) = Stigi.Store.buildIndexPath(dirCap, dirBuf, dirStart, dirEnd, pathBuf) else
    case Missing => RunFileError
  if idxPathLen >= 4096 then RunFileError else
    val LoadedBytes(indexLen) = Stigi.Posix.readFileBytes{region}(idxPathLen, pathBuf, 65536, indexBuf, 0) else
      case LoadFailed => RunFileError
      case LoadOverflow => RunFileError
    if indexLen > 65536 then RunFileError else
      var linePos: U64 = 0
      var unitCount: U64 = 0
      var outcome: Stigi.RunOutcome = RunSuccess
      while linePos < indexLen && outcome == RunSuccess do
        @<lessOrEqual(linePos, indexLen), lessOrEqual(unitCount, Stigi.Limit.maxUnits)>
        val nameStart: U64 = linePos
        while linePos < indexLen && indexBuf(linePos) != 9 && indexBuf(linePos) != 10 do
          @<lessOrEqual(nameStart, linePos), lessOrEqual(linePos, indexLen)>
          linePos = linePos + 1
        val nameEnd: U64 = linePos
        while linePos < indexLen && indexBuf(linePos) != 10 do
          @<lessOrEqual(linePos, indexLen)>
          linePos = linePos + 1
        if linePos < indexLen then linePos = linePos + 1 else ()
        if nameStart < nameEnd then
          val builtPath: Stigi.Lookup = Stigi.Store.buildUnitPath(dirCap, dirBuf, dirStart, dirEnd, 65536, indexBuf, nameStart, nameEnd, pathBuf)
          val fileLenRes: Stigi.Posix.LoadResult = builtPath match
            case Found(unitPathLen) =>
              if unitPathLen < 4096 then Stigi.Posix.readFileBytes{region}(unitPathLen, pathBuf, 65536, unitBuf, 0) else LoadFailed
            case Missing => LoadFailed
          val nextOutcome: Stigi.RunOutcome = fileLenRes match
            case LoadedBytes(srcLen) =>
              if srcLen <= 65536 then
                Stigi.Door.verifyUnit{region}(65536, unitBuf, srcLen, 16384, toks, 16384, nodes, 16384, facts, 16384, rows, rules) match
                  case DoorAccepted(_, _, _) => RunSuccess
                  case DoorGateFailed => RunDoorRejected
                  case DoorLexFailed => RunDoorRejected
                  case DoorParseFailed => RunDoorRejected
                  case DoorDatalogRejected(_) => RunDoorRejected
                  case DoorOutOfMemory => RunOutOfMemory
              else RunFileError
            case LoadFailed => RunFileError
            case LoadOverflow => RunFileError
          outcome = nextOutcome
          if unitCount < Stigi.Limit.maxUnits then unitCount = unitCount + 1 else outcome = RunFileError
        else ()
      if outcome == RunSuccess && unitCount == 0 then RunDoorRejected else outcome
