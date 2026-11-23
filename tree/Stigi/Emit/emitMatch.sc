// prompt: emit match scrutinee bindings and iterate case arms via Stigi.Emit.emitMatchArm.
def Stigi.Emit.emitMatch {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), scrutBeg: U64, scrutEnd: U64, armsBeg: U64, armsEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(scrutBeg, scrutEnd), lessOrEqual(scrutEnd, tokCap), lessOrEqual(armsBeg, armsEnd), lessOrEqual(armsEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
    val Filled(scrutTemps) = Array.tryFill[U64]{region}(4, 0) else
      case FillFailed => ()
    val Filled(paramBegs) = Array.tryFill[U64]{region}(4, 0) else
      case FillFailed => ()
    val Filled(paramFins) = Array.tryFill[U64]{region}(4, 0) else
      case FillFailed => ()
    val Filled(argBegs) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val Filled(argFins) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    var scrutCount: U64 = 0
    if scrutBeg < scrutEnd then
      val Token(firstScrutKind, _, _, _, _) = toks(scrutBeg)
      if firstScrutKind == TokLeftParen then
        val tupCount: U64 = Stigi.Emit.splitArgs(tokCap, toks, scrutBeg, scrutEnd, argBegs, argFins)
        var idx: U64 = 0
        while idx < tupCount && idx < 4 && scrutCount < 4 do
          @<lessOrEqual(idx, tupCount), lessOrEqual(idx, 4), lessOrEqual(scrutCount, 4)>
          val segBeg: U64 = argBegs(idx)
          val segFin: U64 = argFins(idx)
          if segBeg <= segFin && segFin <= scrutEnd && monoFin <= srcCap then
            val tid: U64 = writer.nextTemp()
            val infType: Stigi.Emit.InferredType = Stigi.Emit.inferType(srcCap, src, tokCap, toks, segBeg, segFin, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
            Stigi.Emit.emitInferredCType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, infType)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " _sc")
            Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
            Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, segBeg, segFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
            scrutTemps(scrutCount) := tid
            scrutCount = scrutCount + 1
          else ()
          idx = idx + 1
      else if monoFin <= srcCap then
        val tid: U64 = writer.nextTemp()
        val infType: Stigi.Emit.InferredType = Stigi.Emit.inferType(srcCap, src, tokCap, toks, scrutBeg, scrutEnd, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.emitInferredCType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, infType)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " _sc")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, scrutBeg, scrutEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        scrutTemps(0) := tid
        scrutCount = 1
      else ()
    else
      var locIdx: U64 = 0
      while locIdx < localCount && scrutCount < 4 do
        @<lessOrEqual(locIdx, localCount), lessOrEqual(scrutCount, 4)>
        val FieldInfo(locKind, locBeg, locFin, _, _, _, _) = locals(locIdx)
        if locKind == FieldParam || locKind == FieldLocal || locKind == FieldLocalArray then
          paramBegs(scrutCount) := locBeg
          paramFins(scrutCount) := locFin
          scrutCount = scrutCount + 1
        else ()
        locIdx = locIdx + 1
    val flagNum: U64 = writer.nextTemp()
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 11, "  bool _mat")
    Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, flagNum)
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 9, " = false;")
    var pos: U64 = armsBeg
    var caseCol: U64 = 0
    var hasCaseCol: Bool = false
    while pos < armsEnd do
      @<lessOrEqual(armsBeg, pos), lessOrEqual(pos, armsEnd)>
      val Token(curKind, _, _, _, curCol) = toks(pos)
      if curKind == TokNewline || curKind == TokEnd then pos = pos + 1 else if curKind == TokCase && (!hasCaseCol || curCol == caseCol) then
        if !hasCaseCol then
          caseCol = curCol
          hasCaseCol = true
        else ()
        val patStart: U64 = pos + 1
        var scanPos: U64 = patStart
        var arrowPos: U64 = armsEnd
        var guardPos: U64 = armsEnd
        var foundArrow: Bool = false
        while scanPos < armsEnd && !foundArrow do
          @<lessOrEqual(patStart, scanPos), lessOrEqual(scanPos, armsEnd), lessOrEqual(patStart, guardPos), lessOrEqual(guardPos, armsEnd), lessOrEqual(patStart, arrowPos), lessOrEqual(arrowPos, armsEnd)>
          val Token(scanKind, _, _, _, _) = toks(scanPos)
          if scanKind == TokIf && guardPos == armsEnd then guardPos = scanPos else if scanKind == TokArrow then
            arrowPos = scanPos
            foundArrow = true
          else ()
          scanPos = scanPos + 1
        val armBodyStart: U64 = if arrowPos < armsEnd then arrowPos + 1 else armsEnd
        var armBodyEnd: U64 = armBodyStart
        var armScanDone: Bool = false
        while armBodyEnd < armsEnd && !armScanDone do
          @<lessOrEqual(armBodyStart, armBodyEnd), lessOrEqual(armBodyEnd, armsEnd)>
          val Token(endKind, _, _, _, endCol) = toks(armBodyEnd)
          if endKind == TokCase && endCol <= caseCol then armScanDone = true else if endKind == TokEnd then armScanDone = true else armBodyEnd = armBodyEnd + 1
        val patEnd: U64 = if guardPos < arrowPos then guardPos else arrowPos
        Stigi.Emit.emitMatchArm{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, patStart, patEnd, guardPos, arrowPos, armBodyStart, armBodyEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, scrutTemps, paramBegs, paramFins, flagNum, meta, target, nextFuel)
        pos = armBodyEnd
      else
        pos = pos + 1
  else ()
