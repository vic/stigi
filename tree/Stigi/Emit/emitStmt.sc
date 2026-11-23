// prompt: emit an indented block of L0 statements by dispatching to focused per-form statement emitters.
def Stigi.Emit.emitStmt {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), initLocals: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(initLocals, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    var pos: U64 = startTok
    var locCount: U64 = initLocals
    var baseCol: U64 = 0
    var hasBaseCol: Bool = false
    var stopped: Bool = false
    while pos < endTok && !stopped do
      @<lessOrEqual(startTok, pos), lessOrEqual(pos, endTok), lessOrEqual(locCount, localCap)>
      val Token(leadKind, _, _, _, leadCol) = toks(pos)
      if leadKind == TokNewline || leadKind == TokEnd then
        pos = pos + 1
      else if leadKind == TokAt then
        pos = Stigi.Emit.passAngles(tokCap, toks, pos + 1, endTok)
      else if leadKind == TokCase then
        Stigi.Emit.emitMatch{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, pos, pos, endTok, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, target, nextFuel)
        stopped = true
      else
        if !hasBaseCol then
          baseCol = leadCol
          hasBaseCol = true
        else ()
        val stmtEnd: U64 = Stigi.Emit.findStmtEnd(tokCap, toks, pos, endTok, baseCol)
        if stmtEnd <= pos then
          pos = pos + 1
        else if leadKind == TokVal || leadKind == TokVar then
          val bindRes: Stigi.Emit.ValBindOutcome = Stigi.Emit.emitValBind{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, endTok, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, target, nextFuel)
          val consumed: Bool = bindRes match
            case BindConsumed => true
            case BindContinue(_) => false
          val nextLocs: U64 = bindRes match
            case BindConsumed => locCount
            case BindContinue(cnt) => cnt
          if consumed then stopped = true else
            if nextLocs <= localCap then locCount = nextLocs else ()
            pos = stmtEnd
        else if leadKind == TokHandle then
          locCount = Stigi.Emit.emitHandleStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
          pos = stmtEnd
        else if leadKind == TokWhile then
          var loopBodyPos: U64 = stmtEnd
          var scanPos: U64 = pos + 1
          while scanPos < stmtEnd && loopBodyPos == stmtEnd do
            @<lessOrEqual(pos, scanPos), lessOrEqual(scanPos, stmtEnd)>
            val Token(curKind, _, _, _, _) = toks(scanPos)
            if curKind == TokLoopBody then loopBodyPos = scanPos else scanPos = scanPos + 1
          val condBeg: U64 = pos + 1
          val bodyBeg: U64 = if loopBodyPos < stmtEnd then loopBodyPos + 1 else stmtEnd
          if condBeg <= loopBodyPos then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "  while (")
            Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, condBeg, loopBodyPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
            Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, bodyBeg, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, TargetDiscard, nextFuel)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "  }")
          else ()
          pos = stmtEnd
        else if leadKind == TokIf then
          var remCheck: U64 = stmtEnd
          var hasMore: Bool = false
          while remCheck < endTok && !hasMore do
            @<lessOrEqual(stmtEnd, remCheck), lessOrEqual(remCheck, endTok)>
            val Token(remKind, _, _, _, _) = toks(remCheck)
            if remKind != TokNewline && remKind != TokEnd then hasMore = true else ()
            remCheck = remCheck + 1
          val branchTarget: Stigi.Emit.TargetKind = if hasMore then TargetDiscard else target
          var curIf: U64 = pos
          var closeBraces: U64 = 0
          var chainDone: Bool = false
          while !chainDone && closeBraces < 64 && curIf < stmtEnd do
            @<lessOrEqual(pos, curIf), lessOrEqual(curIf, stmtEnd), lessOrEqual(closeBraces, 64)>
            val splitRes: Stigi.Emit.IfSplit = Stigi.Emit.findIfParts(tokCap, toks, curIf, stmtEnd)
            val isSplitValid: Bool = splitRes match
              case IfParts(_, _) => true
              case IfIncomplete => false
            val thenPos: U64 = splitRes match
              case IfParts(thenSlot, _) => thenSlot
              case IfIncomplete => stmtEnd
            val elsePos: U64 = splitRes match
              case IfParts(_, elseSlot) => elseSlot
              case IfIncomplete => stmtEnd
            if isSplitValid && curIf < thenPos && thenPos < elsePos && elsePos < stmtEnd then
              val condBeg: U64 = curIf + 1
              val thenBeg: U64 = thenPos + 1
              var elseBeg: U64 = elsePos + 1
              var trimElse: Bool = true
              while elseBeg < stmtEnd && trimElse do
                @<lessOrEqual(elsePos, elseBeg), lessOrEqual(elseBeg, stmtEnd)>
                val Token(elseLead, _, _, _, _) = toks(elseBeg)
                if elseLead == TokNewline then elseBeg = elseBeg + 1 else trimElse = false
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "  if (")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, condBeg, thenPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
              Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
              Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, thenBeg, elsePos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, branchTarget, nextFuel)
              Stigi.Emit.writeLine{writer}(outCap, outBuf, 10, "  } else {")
              closeBraces = closeBraces + 1
              val nextIsIf: Bool = if elseBeg < stmtEnd then
                val Token(nextLead, _, _, _, _) = toks(elseBeg)
                nextLead == TokIf
              else false
              if nextIsIf then
                curIf = elseBeg
              else
                Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, elseBeg, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, branchTarget, nextFuel)
                chainDone = true
            else
              chainDone = true
          while closeBraces > 0 do
            @<lessOrEqual(closeBraces, 64)>
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "  }")
            closeBraces = closeBraces - 1
          pos = stmtEnd
        else
          var remCheck: U64 = stmtEnd
          var hasMore: Bool = false
          while remCheck < endTok && !hasMore do
            @<lessOrEqual(stmtEnd, remCheck), lessOrEqual(remCheck, endTok)>
            val Token(remKind, _, _, _, _) = toks(remCheck)
            if remKind != TokNewline && remKind != TokEnd then hasMore = true else ()
            remCheck = remCheck + 1
          Stigi.Emit.emitExprStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, hasMore, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, target, nextFuel)
          pos = stmtEnd
  else ()
