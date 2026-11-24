// prompt: verify one unit through every door stage using caller-provided reusable scratch buffers.
pub def Stigi.Door.verifyUnit {region: Stigi.Region, Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), factCap: Stigi.NodeCap, facts: Array[Stigi.Extract.FactRow](factCap), rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.Store.Tuple](rowCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), smallSlots: Array[Stigi.Datalog.Store.Slot](512)) <lessOrEqual(srcLen, srcCap), lessOrEqual(factCap, rowCap), below(0, rowCap)>: Stigi.Door.Verdict =
  val gateRes: Stigi.TenetGate.GateOutcome = Stigi.TenetGate.checkSource(srcCap, src, srcLen)
  val gateValid: Bool = gateRes match
    case GateClean => true
    case GateMissingPrompt => false
    case GateBannedWord(_) => false
    case GateCrypticName(_) => false
    case GateTrailingDigit(_) => false
  if !gateValid then DoorGateFailed else
    val LexDone(tokCount) = Stigi.Lex.scanAll(srcCap, src, srcLen, tokCap, toks) else
      case LexOverflow => DoorLexFailed
      case LexSyntaxError(_, _) => DoorLexFailed
    if tokCount <= tokCap then
      val ParsedUnit(_, nodeCount) = Stigi.Parse.parseUnit(srcCap, src, tokCap, toks, tokCount, nodeCap, nodes) else
        case ParseFailed(_) => DoorParseFailed
      if nodeCount <= nodeCap then
        val Hashed(_) = Stigi.Hash.hashBytes{region}(srcCap, src, srcLen) else
          case HashOutOfMemory => DoorOutOfMemory
        val factCount: U64 = Stigi.Extract.extractAst(tokCap, toks, tokCount, nodeCap, nodes, nodeCount, factCap, facts)
        if factCount == 0 then
          DoorAccepted(nodeCount, 0, 0)
        else if factCount <= 128 then
          var clearIdx: U64 = 0
          while clearIdx < 512 do
            @<lessOrEqual(clearIdx, 512)>
            val nextClear: U64 = clearIdx + 1
            smallSlots(clearIdx) := StoreVacant
            clearIdx = nextClear
          Stigi.Door.runEngineCheck{region}(factCap, facts, factCount, nodeCount, rowCap, rows, nextInRel, 512, smallSlots)
        else
          val Filled(bigSlots) = Array.tryFill[Stigi.Datalog.Store.Slot]{region}(rowCap, StoreVacant) else
            case FillFailed => DoorOutOfMemory
          Stigi.Door.runEngineCheck{region}(factCap, facts, factCount, nodeCount, rowCap, rows, nextInRel, rowCap, bigSlots)
      else DoorParseFailed
    else DoorLexFailed
