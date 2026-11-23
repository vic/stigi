// prompt: emit a handle h = Handler(args) statement allocating handler state and binding a capability handle.
def Stigi.Emit.emitHandleStmt {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), pos: U64, stmtEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, fuel: U64) <below(0, outCap), lessOrEqual(pos, stmtEnd), lessOrEqual(stmtEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: U64 <lessOrEqual(result, localCap)> =
  val varPos: U64 = pos + 1
  val rhsPos: U64 = pos + 3
  if fuel > 0 && varPos < stmtEnd && rhsPos < stmtEnd then
    val nextFuel: U64 = fuel - 1
    var locCount: U64 = localCount
    val Token(_, hndVarBeg, hndVarFin, _, _) = toks(varPos)
    val TypeSpan(hndNameBeg, hndNameFin, _, _, afterHndName) = Stigi.Emit.parseTypeSpan(tokCap, toks, rhsPos, stmtEnd)
    val callPos: U64 = if afterHndName <= stmtEnd then afterHndName else stmtEnd
    val Filled(argBegs) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => locCount
    val Filled(argFins) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => locCount
    val argCount: U64 = Stigi.Emit.splitArgs(tokCap, toks, callPos, stmtEnd, argBegs, argFins)
    if hndVarFin <= srcCap && hndNameFin <= srcCap then
      val stateTid: U64 = writer.nextTemp()
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndNameBeg, hndNameFin)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "_State _hs")
      Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, stateTid)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndNameBeg, hndNameFin)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "_init(")
      var argIdx: U64 = 0
      while argIdx < argCount do
        @<lessOrEqual(argIdx, argCount)>
        if argIdx > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
        val argBeg: U64 = argBegs(argIdx)
        val argFin: U64 = argFins(argIdx)
        if argBeg <= argFin && argFin <= stmtEnd then
          Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argBeg, argFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
        else ()
        argIdx = argIdx + 1
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
      var capBeg: U64 = 0
      var capFin: U64 = 0
      var tagNum: U64 = 0
      var symIdx: U64 = 0
      var foundHnd: Bool = false
      while symIdx < symCount && !foundHnd do
        @<lessOrEqual(symIdx, symCount), lessOrEqual(capBeg, capFin), lessOrEqual(capFin, srcCap)>
        val SymEntry(symKind, _, symBeg, symFin, targetBeg, targetFin, _, _, _, _) = syms(symIdx)
        if symKind == SymHandler && symFin <= srcCap && targetFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, hndNameBeg, hndNameFin, symBeg, symFin) then
          foundHnd = true
          capBeg = targetBeg
          capFin = targetFin
        else ()
        symIdx = symIdx + 1
      var countIdx: U64 = 0
      var tagDone: Bool = false
      while countIdx < symCount && !tagDone do
        @<lessOrEqual(countIdx, symCount)>
        val SymEntry(symKind, _, symBeg, symFin, targetBeg, targetFin, _, _, _, _) = syms(countIdx)
        if symKind == SymHandler && symFin <= srcCap && targetFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, capBeg, capFin, targetBeg, targetFin) then
          if Stigi.Emit.sliceEqual(srcCap, src, hndNameBeg, hndNameFin, symBeg, symFin) then tagDone = true else if tagNum < symCap then tagNum = tagNum + 1 else ()
        else ()
        countIdx = countIdx + 1
      if foundHnd then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, capBeg, capFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "_Handle v_")
        writer.putSlice(outCap, outBuf, srcCap, src, hndVarBeg, hndVarFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 12, " = { .tag = ")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tagNum)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 15, "U, .ptr = { .h_")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tagNum)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, " = &_hs")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, stateTid)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 5, " } };")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  (void)v_")
        writer.putSlice(outCap, outBuf, srcCap, src, hndVarBeg, hndVarFin)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        if locCount < localCap then
          locals(locCount) := FieldInfo(FieldLocalCap, hndVarBeg, hndVarFin, capBeg, capFin, 0, 0)
          locCount = locCount + 1
        else ()
      else ()
    else ()
    locCount
  else localCount
