// prompt: verify one unit through every door stage using caller-provided reusable scratch buffers.
pub def Stigi.Door.verifyUnit {region: Stigi.Region, Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), factCap: Stigi.NodeCap, facts: Array[Stigi.Extract.FactRow](factCap), rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), rules: Array[Stigi.Datalog.RuleSpec](4)) <lessOrEqual(srcLen, srcCap), lessOrEqual(factCap, rowCap), below(0, rowCap)>: Stigi.Door.Verdict =
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
        var idx: U64 = 0
        while idx < factCount do
          @<lessOrEqual(idx, factCount)>
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
          rows(idx) := TupleRow(relTag, colZero, colOne, colTwo)
          idx = idx + 1
        rules(0) := RuleSpec(1, 1, 2, JoinChain)
        rules(1) := RuleSpec(2, 1, 0, JoinCopy)
        rules(2) := RuleSpec(99, 3, 1, JoinAntiMatch)
        Stigi.Datalog.runSemiNaive{region}(rowCap, rows, factCount, 4, rules, 3, 99) match
          case EngineConverged(derivedRows, rejectCount) =>
            if rejectCount == 0 then DoorAccepted(nodeCount, factCount, derivedRows) else DoorDatalogRejected(rejectCount)
          case EngineBudgetExceeded => DoorDatalogRejected(1)
          case EngineOutOfMemory => DoorOutOfMemory
      else DoorParseFailed
    else DoorLexFailed
