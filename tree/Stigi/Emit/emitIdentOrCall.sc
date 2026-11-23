// prompt: emit an identifier, array index, capability method call, enum constructor, builtin, or function call.
def Stigi.Emit.emitIdentOrCall {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, fuel: U64) <below(0, outCap), lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 && startTok < endTok then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
    val TypeSpan(qualBeg, qualFin, _, _, afterQual) = Stigi.Emit.parseTypeSpan(tokCap, toks, startTok, endTok)
    var callPos: U64 = if afterQual <= endTok then afterQual else endTok
    var capArgBeg: U64 = callPos
    var capArgEnd: U64 = callPos
    if callPos < endTok then
      val Token(braceKind, _, _, _, _) = toks(callPos)
      if braceKind == TokLeftBrace then
        val braceStart: U64 = callPos + 1
        capArgBeg = braceStart
        capArgEnd = braceStart
        var scanPos: U64 = braceStart
        var doneBrace: Bool = false
        while scanPos < endTok && !doneBrace do
          @<lessOrEqual(braceStart, scanPos), lessOrEqual(scanPos, endTok)>
          val Token(curKind, _, _, _, _) = toks(scanPos)
          if curKind == TokRightBrace then
            capArgEnd = scanPos
            callPos = scanPos + 1
            doneBrace = true
          else scanPos = scanPos + 1
      else ()
    else ()
    val Filled(argBegs) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val Filled(argFins) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val hasCall: Bool = if callPos < endTok then
      val Token(parKind, _, _, _, _) = toks(callPos)
      parKind == TokLeftParen
    else false
    val argCount: U64 = if hasCall && callPos <= endTok then Stigi.Emit.splitArgs(tokCap, toks, callPos, endTok, argBegs, argFins) else 0
    val calleeKind: Stigi.Emit.CalleeKind = if qualFin <= srcCap then Stigi.Emit.classifyCallee(srcCap, src, qualBeg, qualFin) else CalleeNormal
    val isSingleCast: Bool = calleeKind == CalleeCastWord || calleeKind == CalleeCastWordShort || calleeKind == CalleeCastByte || calleeKind == CalleeCastSignedLong || calleeKind == CalleeCastSignedWord
    val isBinPrim: Bool = calleeKind == CalleeWrapAddWord || calleeKind == CalleeWrapSubWord || calleeKind == CalleeWrapMulWord || calleeKind == CalleeBitAndWord || calleeKind == CalleeBitOrWord || calleeKind == CalleeBitXorWord || calleeKind == CalleeShiftLeftWord || calleeKind == CalleeShiftRightWord || calleeKind == CalleeWrapAddShort || calleeKind == CalleeWrapSubShort || calleeKind == CalleeWrapMulShort || calleeKind == CalleeBitAndShort || calleeKind == CalleeBitOrShort || calleeKind == CalleeBitXorShort || calleeKind == CalleeShiftLeftShort || calleeKind == CalleeShiftRightShort
    val argZeroBeg: U64 = argBegs(0)
    val argZeroFin: U64 = argFins(0)
    val argOneBeg: U64 = argBegs(1)
    val argOneFin: U64 = argFins(1)
    if isSingleCast && argCount >= 1 && argZeroBeg <= argZeroFin && argZeroFin <= endTok then
      Stigi.Emit.emitPrimParts{writer}(outCap, outBuf, calleeKind, 0)
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      Stigi.Emit.emitPrimParts{writer}(outCap, outBuf, calleeKind, 2)
    else if isBinPrim && argCount >= 2 && argZeroBeg <= argZeroFin && argZeroFin <= endTok && argOneBeg <= argOneFin && argOneFin <= endTok then
      Stigi.Emit.emitPrimParts{writer}(outCap, outBuf, calleeKind, 0)
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      Stigi.Emit.emitPrimParts{writer}(outCap, outBuf, calleeKind, 1)
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argOneBeg, argOneFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      Stigi.Emit.emitPrimParts{writer}(outCap, outBuf, calleeKind, 2)
    else if calleeKind == CalleeMixWord && argCount >= 1 && argZeroBeg <= argZeroFin && argZeroFin <= endTok then
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 24, "Stigi_Stigi_Raw_mixWord(")
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      writer.putByte(outCap, outBuf, 41)
    else if calleeKind == CalleeRotateRightShort && argCount >= 2 && argZeroBeg <= argZeroFin && argZeroFin <= endTok && argOneBeg <= argOneFin && argOneFin <= endTok then
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 32, "Stigi_Stigi_Raw_rotateRightWord(")
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argOneBeg, argOneFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      writer.putByte(outCap, outBuf, 41)
    else if calleeKind == CalleeTraitEqual || calleeKind == CalleeTraitLess then
      var foundGiv: Bool = false
      var givBeg: U64 = 0
      var givFin: U64 = 0
      var symIdx: U64 = 0
      while symIdx < symCount && !foundGiv do
        @<lessOrEqual(symIdx, symCount), lessOrEqual(givBeg, givFin), lessOrEqual(givFin, srcCap)>
        val SymEntry(symKind, _, symBeg, symFin, tgtBeg, tgtFin, _, _, _, _) = syms(symIdx)
        if symKind == SymGiven && symFin <= srcCap && tgtFin <= srcCap && monoFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, tgtBeg, tgtFin, monoBeg, monoFin) then
          val isOrderGiv: Bool = Stigi.Lex.sliceEquals(srcCap, src, symBeg, symFin, 16, "Stigi.Order.Word")
          if calleeKind == CalleeTraitLess && isOrderGiv || calleeKind == CalleeTraitEqual && !isOrderGiv then
            foundGiv = true
            givBeg = symBeg
            givFin = symFin
          else ()
        else ()
        symIdx = symIdx + 1
      if foundGiv && argCount >= 2 && argZeroBeg <= argZeroFin && argZeroFin <= endTok && argOneBeg <= argOneFin && argOneFin <= endTok then
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, givBeg, givFin)
        if calleeKind == CalleeTraitEqual then Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "_equal(") else Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "_lessThan(")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argOneBeg, argOneFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        writer.putByte(outCap, outBuf, 41)
      else Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, "false")
    else if calleeKind == CalleeAbiOpenat || calleeKind == CalleeAbiUnlink || calleeKind == CalleeAbiClock || calleeKind == CalleeAbiUsage || calleeKind == CalleeAbiDirect then
      if qualFin <= srcCap then
        writer.putSlice(outCap, outBuf, srcCap, src, qualBeg, qualFin)
        writer.putByte(outCap, outBuf, 40)
        var argIdx: U64 = 0
        while argIdx < argCount do
          @<lessOrEqual(argIdx, argCount)>
          if argIdx > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
          if calleeKind == CalleeAbiOpenat && argIdx == 1 || calleeKind == CalleeAbiUnlink && argIdx == 0 then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, "(const char *)")
          else if calleeKind == CalleeAbiClock && argIdx == 0 then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 11, "(clockid_t)")
          else if calleeKind == CalleeAbiClock && argIdx == 1 then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 27, "(struct timespec *)(void *)")
          else if calleeKind == CalleeAbiUsage && argIdx == 1 then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 25, "(struct rusage *)(void *)")
          else ()
          val argBeg: U64 = argBegs(argIdx)
          val argFin: U64 = argFins(argIdx)
          if argBeg <= argFin && argFin <= endTok then
            Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argBeg, argFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
          else ()
          argIdx = argIdx + 1
        writer.putByte(outCap, outBuf, 41)
      else ()
    else if capArgBeg <= capArgEnd && capArgEnd <= endTok then
      Stigi.Emit.emitUserRef{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, startTok, endTok, qualBeg, qualFin, capArgBeg, capArgEnd, hasCall, argCount, argBegs, argFins, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
    else ()
  else Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
