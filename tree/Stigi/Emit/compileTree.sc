// prompt: load, verify through Stigi.Door, index, emit deterministic ISO C11, and write output file.
pub def Stigi.Emit.compileTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outPathLen: U64, outPath: Array[U8](4096)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), below(outPathLen, 4096)>: Stigi.RunOutcome =
  handle writer = Stigi.Emit.BufferWriter(2097152)
  val Filled(pathBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(indexBuf) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(unitBuf) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(srcBuf) = Array.tryFill[U8]{region}(1048576, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(allToks) = Array.tryFill[Stigi.Lex.Token]{region}(131072, Token(TokEnd, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(unitToks) = Array.tryFill[Stigi.Lex.Token]{region}(8192, Token(TokEnd, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(nodes) = Array.tryFill[Stigi.Ast.Node]{region}(8192, Node(ExprUnit, 0, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(facts) = Array.tryFill[Stigi.Extract.FactRow]{region}(8192, FactRow(FactCall, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(rows) = Array.tryFill[Stigi.Datalog.TupleRow]{region}(8192, TupleRow(0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(rules) = Array.tryFill[Stigi.Datalog.RuleSpec]{region}(4, RuleSpec(0, 0, 0, JoinCopy)) else
    case FillFailed => RunOutOfMemory
  val Filled(syms) = Array.tryFill[Stigi.Emit.SymEntry]{region}(4096, SymEntry(SymFunc, 0, 0, 0, 0, 0, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(fields) = Array.tryFill[Stigi.Emit.FieldInfo]{region}(8192, FieldInfo(FieldParam, 0, 0, 0, 0, 0, 0)) else
    case FillFailed => RunOutOfMemory
  val Filled(outBuf) = Array.tryFill[U8]{region}(2097152, 0) else
    case FillFailed => RunOutOfMemory
  val Found(idxPathLen) = Stigi.Store.buildIndexPath(dirCap, dirBuf, dirStart, dirEnd, pathBuf) else
    case Missing => RunFileError
  if idxPathLen >= 4096 then RunFileError else
    val LoadedBytes(indexLen) = Stigi.Posix.readFileBytes{region}(idxPathLen, pathBuf, 65536, indexBuf, 0) else
      case LoadFailed => RunFileError
      case LoadOverflow => RunFileError
    if indexLen > 65536 then RunFileError else
      var linePos: U64 = 0
      var srcOff: U64 = 0
      var tokOff: U64 = 0
      var symCount: U64 = 0
      var fieldCount: U64 = 0
      var unitCount: U64 = 0
      var outcome: Stigi.RunOutcome = RunSuccess
      while linePos < indexLen && outcome == RunSuccess do
        @<lessOrEqual(linePos, indexLen), lessOrEqual(srcOff, 1048576), lessOrEqual(tokOff, 131072), lessOrEqual(symCount, 4096), lessOrEqual(fieldCount, 8192), lessOrEqual(unitCount, Stigi.Limit.maxUnits)>
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
          val loadRes: Stigi.Posix.LoadResult = builtPath match
            case Found(unitPathLen) =>
              if unitPathLen < 4096 then Stigi.Posix.readFileBytes{region}(unitPathLen, pathBuf, 65536, unitBuf, 0) else LoadFailed
            case Missing => LoadFailed
          val unitBytes: U64 = loadRes match
            case LoadedBytes(gotBytes) => gotBytes
            case LoadFailed => 0
            case LoadOverflow => 0
          val loadValidFlag: Bool = loadRes match
            case LoadedBytes(_) => true
            case LoadFailed => false
            case LoadOverflow => false
          if !loadValidFlag || unitBytes > 65536 then
            outcome = RunFileError
          else
            val nextSrcOff: U64 = srcOff + unitBytes
            if nextSrcOff > 1048576 then
              outcome = RunFileError
            else
              val doorVerdict: Stigi.Door.Verdict = Stigi.Door.verifyUnit{region}(65536, unitBuf, unitBytes, 8192, unitToks, 8192, nodes, 8192, facts, 8192, rows, rules)
              val doorPassed: Bool = doorVerdict match
                case DoorAccepted(_, _, _) => true
                case DoorGateFailed => false
                case DoorLexFailed => false
                case DoorParseFailed => false
                case DoorDatalogRejected(_) => false
                case DoorOutOfMemory => false
              if !doorPassed then
                outcome = RunDoorRejected
              else
                var copyIdx: U64 = 0
                while copyIdx < unitBytes do
                  @<lessOrEqual(copyIdx, unitBytes)>
                  val absPos: U64 = srcOff + copyIdx
                  if absPos < 1048576 then srcBuf(absPos) := unitBuf(copyIdx) else ()
                  copyIdx = copyIdx + 1
                val lexRes: Stigi.Lex.LexResult = Stigi.Lex.scanAll(65536, unitBuf, unitBytes, 8192, unitToks)
                val tokLen: U64 = lexRes match
                  case LexDone(cnt) => cnt
                  case LexOverflow => 0
                  case LexSyntaxError(_, _) => 0
                val lexValid: Bool = lexRes match
                  case LexDone(_) => true
                  case LexOverflow => false
                  case LexSyntaxError(_, _) => false
                if !lexValid || tokLen > 8192 || tokOff + tokLen > 131072 then
                  outcome = RunEmitError
                else
                  val unitTokStart: U64 = tokOff
                  var tokIdx: U64 = 0
                  while tokIdx < tokLen do
                    @<lessOrEqual(tokIdx, tokLen), lessOrEqual(unitTokStart, tokOff), lessOrEqual(tokOff, 131072)>
                    val Token(tokKind, relBeg, relFin, lineNum, colNum) = unitToks(tokIdx)
                    if relFin <= 65536 && tokOff < 131072 then
                      val absBeg: U64 = srcOff + relBeg
                      val absFin: U64 = srcOff + relFin
                      if absBeg <= absFin then
                        allToks(tokOff) := Token(tokKind, absBeg, absFin, lineNum, colNum)
                        tokOff = tokOff + 1
                      else ()
                    else ()
                    tokIdx = tokIdx + 1
                  val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.indexUnit(1048576, srcBuf, 131072, allToks, unitTokStart, tokOff, 4096, syms, symCount, 8192, fields, fieldCount)
                  if nextSyms <= 4096 then symCount = nextSyms else ()
                  if nextFields <= 8192 then fieldCount = nextFields else ()
                  srcOff = nextSrcOff
                  if unitCount < Stigi.Limit.maxUnits then unitCount = unitCount + 1 else outcome = RunEmitError
        else ()
      if outcome != RunSuccess then outcome else if unitCount == 0 then RunDoorRejected else
        Stigi.Emit.emitPreamble{writer}(2097152, outBuf)
        Stigi.Emit.emitEnums{region, writer}(2097152, outBuf, 1048576, srcBuf, 4096, syms, symCount, 8192, fields, fieldCount)
        Stigi.Emit.emitCapsAndHandlers{region, writer}(2097152, outBuf, 1048576, srcBuf, 4096, syms, symCount, 8192, fields, fieldCount)
        Stigi.Emit.emitProtoDispatch{region, writer}(2097152, outBuf, 1048576, srcBuf, 4096, syms, symCount, 8192, fields, fieldCount)
        Stigi.Emit.emitPrototypes{writer}(2097152, outBuf, 1048576, srcBuf, 131072, allToks, 4096, syms, symCount, 8192, fields, fieldCount)
        Stigi.Emit.emitAllBodies{region, writer}(2097152, outBuf, 1048576, srcBuf, 131072, allToks, 4096, syms, symCount, 8192, fields, fieldCount)
        Stigi.Emit.emitMain{writer}(2097152, outBuf, 1048576, srcBuf, 4096, syms, symCount)
        val outLen: U64 = writer.writtenCount()
        if writer.didOverflow() || outLen > 2097152 then RunEmitError else if Stigi.Posix.writeFileBytes{region}(outPathLen, outPath, 2097152, outBuf, outLen) then RunSuccess else RunFileError
