// prompt: emit val Ctor(...) = expr single-case destructuring and flat exhaustive val-else bindings.
def Stigi.Emit.emitEnumValBind {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), namePos: U64, assignPos: U64, rhsStart: U64, rhsEnd: U64, elsePos: U64, stmtEnd: U64, blockEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(namePos, assignPos), lessOrEqual(assignPos, stmtEnd), lessOrEqual(rhsStart, rhsEnd), lessOrEqual(rhsEnd, stmtEnd), lessOrEqual(elsePos, stmtEnd), lessOrEqual(stmtEnd, blockEnd), lessOrEqual(blockEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Stigi.Emit.ValBindOutcome =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
    if monoFin <= srcCap then
      val tid: U64 = writer.nextTemp()
      val infType: Stigi.Emit.InferredType = Stigi.Emit.inferType(srcCap, src, tokCap, toks, rhsStart, rhsEnd, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
      Stigi.Emit.emitInferredCType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, infType)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " _sc")
      Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsStart, rhsEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
      if elsePos < stmtEnd then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "  if (")
        Stigi.Emit.emitPatCond{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, namePos, assignPos, symCap, syms, symCount, tid, 0, 0)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
        val boundLocs: U64 = Stigi.Emit.emitPatBinds{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, namePos, assignPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, tid, 0, 0)
        Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, stmtEnd, blockEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, boundLocs, meta, target, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 10, "  } else {")
        val elseArmsStart: U64 = elsePos + 1
        var armPos: U64 = elseArmsStart
        while armPos < stmtEnd do
          @<lessOrEqual(elseArmsStart, armPos), lessOrEqual(armPos, stmtEnd)>
          val Token(armKind, _, _, _, armCol) = toks(armPos)
          if armKind == TokCase then
            val patStart: U64 = armPos + 1
            var arrowPos: U64 = stmtEnd
            var scanArm: U64 = patStart
            while scanArm < stmtEnd && arrowPos == stmtEnd do
              @<lessOrEqual(patStart, scanArm), lessOrEqual(scanArm, stmtEnd)>
              val Token(scanKind, _, _, _, _) = toks(scanArm)
              if scanKind == TokArrow then arrowPos = scanArm else scanArm = scanArm + 1
            val bodyBeg: U64 = if arrowPos < stmtEnd then arrowPos + 1 else stmtEnd
            var bodyEnd: U64 = bodyBeg
            var scanEnd: Bool = false
            while bodyEnd < stmtEnd && !scanEnd do
              @<lessOrEqual(bodyBeg, bodyEnd), lessOrEqual(bodyEnd, stmtEnd)>
              val Token(endKind, _, _, _, endCol) = toks(bodyEnd)
              if endKind == TokCase && endCol <= armCol then scanEnd = true else bodyEnd = bodyEnd + 1
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "    if (")
            Stigi.Emit.emitPatCond{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, patStart, arrowPos, symCap, syms, symCount, tid, 0, 0)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
            val failLocs: U64 = Stigi.Emit.emitPatBinds{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, patStart, arrowPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, tid, 0, 0)
            Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, bodyBeg, bodyEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, failLocs, meta, target, nextFuel)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 5, "    }")
            armPos = bodyEnd
          else armPos = armPos + 1
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "  }")
        BindConsumed
      else
        val boundLocs: U64 = Stigi.Emit.emitPatBinds{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, namePos, assignPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, tid, 0, 0)
        BindContinue(boundLocs)
    else BindContinue(localCount)
  else BindContinue(localCount)
