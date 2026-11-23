// prompt: dispatch val/var bindings cleanly across Array.tryFill, enum destructuring, and simple bindings.
def Stigi.Emit.emitValBind {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), stmtBeg: U64, stmtEnd: U64, blockEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(stmtBeg, stmtEnd), lessOrEqual(stmtEnd, blockEnd), lessOrEqual(blockEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Stigi.Emit.ValBindOutcome =
  val namePos: U64 = stmtBeg + 1
  if fuel > 0 && namePos < stmtEnd then
    val nextFuel: U64 = fuel - 1
    val Token(leadKind, _, _, _, _) = toks(stmtBeg)
    val Token(nameKind, nameBeg, nameFin, _, _) = toks(namePos)
    if nameKind == TokUpper then
      var assignPos: U64 = stmtEnd
      var scanAssign: U64 = namePos + 1
      while scanAssign < stmtEnd && assignPos == stmtEnd do
        @<lessOrEqual(namePos, scanAssign), lessOrEqual(scanAssign, stmtEnd)>
        val Token(curKind, _, _, _, _) = toks(scanAssign)
        if curKind == TokAssign then assignPos = scanAssign else scanAssign = scanAssign + 1
      val rhsStart: U64 = if assignPos < stmtEnd then assignPos + 1 else stmtEnd
      var elsePos: U64 = stmtEnd
      var scanElse: U64 = rhsStart
      var parenDepth: U64 = 0
      while scanElse < stmtEnd && elsePos == stmtEnd do
        @<lessOrEqual(rhsStart, scanElse), lessOrEqual(scanElse, stmtEnd), lessOrEqual(parenDepth, Stigi.Limit.maxTokens)>
        val Token(curKind, _, _, _, _) = toks(scanElse)
        if curKind == TokLeftParen && parenDepth < Stigi.Limit.maxTokens then parenDepth = parenDepth + 1 else if curKind == TokRightParen && parenDepth > 0 then parenDepth = parenDepth - 1 else if curKind == TokElse && parenDepth == 0 then elsePos = scanElse else ()
        scanElse = scanElse + 1
      val rhsEnd: U64 = if elsePos < stmtEnd then elsePos else stmtEnd
      val isFilled: Bool = if nameFin <= srcCap then Stigi.Lex.sliceEquals(srcCap, src, nameBeg, nameFin, 6, "Filled") else false
      val bindPos: U64 = namePos + 2
      if isFilled && elsePos < stmtEnd && bindPos < assignPos then
        val Token(_, arrBeg, arrFin, _, _) = toks(bindPos)
        if arrFin <= srcCap then
          Stigi.Emit.emitTryFillBind{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, arrBeg, arrFin, rhsStart, rhsEnd, elsePos, stmtEnd, blockEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, target, nextFuel)
        else ()
        BindConsumed
      else
        Stigi.Emit.emitEnumValBind{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, namePos, assignPos, rhsStart, rhsEnd, elsePos, stmtEnd, blockEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, target, nextFuel)
    else
      val nextLocs: U64 = Stigi.Emit.emitSimpleValBind{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, namePos, stmtEnd, leadKind == TokVar, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      BindContinue(nextLocs)
  else BindContinue(localCount)
