// prompt: recursively emit any pure L0 value expression slice as strict ISO C11 with proved depth bound.
def Stigi.Emit.emitExpr {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, fuel: U64) <below(0, outCap), lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    var beg: U64 = startTok
    var fin: U64 = endTok
    var trimLead: Bool = true
    while beg < fin && trimLead do
      @<lessOrEqual(startTok, beg), lessOrEqual(beg, fin)>
      val Token(kind, _, _, _, _) = toks(beg)
      if kind == TokNewline then beg = beg + 1 else trimLead = false
    var trimTail: Bool = true
    while beg < fin && trimTail do
      @<lessOrEqual(beg, fin), lessOrEqual(fin, endTok)>
      val lastPos: U64 = fin - 1
      val Token(kind, _, _, _, _) = toks(lastPos)
      if kind == TokNewline && beg <= lastPos then fin = lastPos else trimTail = false
    var scanAt: U64 = beg
    var atDepth: U64 = 0
    var cutAt: U64 = fin
    var cutFound: Bool = false
    while scanAt < fin && !cutFound do
      @<lessOrEqual(beg, scanAt), lessOrEqual(scanAt, fin), lessOrEqual(beg, cutAt), lessOrEqual(cutAt, fin), lessOrEqual(atDepth, Stigi.Limit.maxTokens)>
      val Token(atKind, _, _, _, _) = toks(scanAt)
      if atKind == TokLeftParen && atDepth < Stigi.Limit.maxTokens then atDepth = atDepth + 1
      else if atKind == TokRightParen && atDepth > 0 then atDepth = atDepth - 1
      else if atKind == TokAt && atDepth == 0 then
        cutAt = scanAt
        cutFound = true
      else ()
      scanAt = scanAt + 1
    fin = cutAt
    if beg >= fin then Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "((void)0)") else
      val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
      val Token(firstKind, firstBeg, firstFin, _, _) = toks(beg)
      val lastIdx: U64 = fin - 1
      val Token(lastKind, _, _, _, _) = toks(lastIdx)
      var wrappedParen: Bool = false
      if firstKind == TokLeftParen && lastKind == TokRightParen && beg < lastIdx then
        val innerBeg: U64 = beg + 1
        var scanPos: U64 = innerBeg
        var depth: U64 = 1
        var hitsZeroEarly: Bool = false
        while scanPos < lastIdx do
          @<lessOrEqual(innerBeg, scanPos), lessOrEqual(scanPos, lastIdx), lessOrEqual(depth, Stigi.Limit.maxTokens)>
          val Token(scanKind, _, _, _, _) = toks(scanPos)
          if scanKind == TokLeftParen && depth < Stigi.Limit.maxTokens then depth = depth + 1 else if scanKind == TokRightParen && depth > 0 then
            depth = depth - 1
            if depth == 0 then hitsZeroEarly = true else ()
          else ()
          scanPos = scanPos + 1
        if !hitsZeroEarly && depth == 1 then wrappedParen = true else ()
      else ()
      if wrappedParen && beg < lastIdx then
        writer.putByte(outCap, outBuf, 40)
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg + 1, lastIdx, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        writer.putByte(outCap, outBuf, 41)
      else if firstKind == TokIf then
        Stigi.Emit.findIfParts(tokCap, toks, beg, fin) match
          case IfParts(thenPos, elsePos) =>
            if beg < thenPos && thenPos < elsePos && elsePos < fin then
              val condStart: U64 = beg + 1
              val thenStart: U64 = thenPos + 1
              val elseStart: U64 = elsePos + 1
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "((")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, condStart, thenPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, ") ? (")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, thenStart, elsePos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, ") : (")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, elseStart, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "))")
            else Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
          case IfIncomplete => Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
      else
        Stigi.Emit.findTopOper(tokCap, toks, beg, fin) match
          case OperFound(operPos, operKind) =>
            if beg <= operPos && operPos < fin then
              val rhsStart: U64 = operPos + 1
              if operKind == TokPipe then
                val TypeSpan(pipeBeg, pipeFin, _, _, _) = Stigi.Emit.parseTypeSpan(tokCap, toks, rhsStart, fin)
                if pipeFin <= srcCap then
                  Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, pipeBeg, pipeFin)
                  writer.putByte(outCap, outBuf, 40)
                  Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg, operPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                  writer.putByte(outCap, outBuf, 41)
                else ()
              else if operKind == TokEqual || operKind == TokNotEqual then
                val lhsInf: Stigi.Emit.InferredType = if monoFin <= srcCap then Stigi.Emit.inferType(srcCap, src, tokCap, toks, beg, operPos, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin) else InferUnknown
                val useInf: Stigi.Emit.InferredType = lhsInf match
                  case InferEnum(_, _, _) => lhsInf
                  case InferBool => if monoFin <= srcCap then Stigi.Emit.inferType(srcCap, src, tokCap, toks, rhsStart, fin, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin) else InferUnknown
                  case InferWord => lhsInf
                  case InferWordShort => lhsInf
                  case InferByte => lhsInf
                  case InferSignedLong => lhsInf
                  case InferSignedWord => lhsInf
                  case InferNamed(_, _) => lhsInf
                  case InferUnknown => if monoFin <= srcCap then Stigi.Emit.inferType(srcCap, src, tokCap, toks, rhsStart, fin, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin) else InferUnknown
                val enumCmpFlag: Bool = useInf match
                  case InferEnum(_, _, _) => true
                  case InferBool => false
                  case InferWord => false
                  case InferWordShort => false
                  case InferByte => false
                  case InferSignedLong => false
                  case InferSignedWord => false
                  case InferNamed(_, _) => false
                  case InferUnknown => false
                if enumCmpFlag then
                  val enumBeg: U64 = useInf match
                    case InferEnum(_, begPos, _) => begPos
                    case InferBool => 0
                    case InferWord => 0
                    case InferWordShort => 0
                    case InferByte => 0
                    case InferSignedLong => 0
                    case InferSignedWord => 0
                    case InferNamed(_, _) => 0
                    case InferUnknown => 0
                  val enumFin: U64 = useInf match
                    case InferEnum(_, _, finPos) => finPos
                    case InferBool => 0
                    case InferWord => 0
                    case InferWordShort => 0
                    case InferByte => 0
                    case InferSignedLong => 0
                    case InferSignedWord => 0
                    case InferNamed(_, _) => 0
                    case InferUnknown => 0
                  if enumBeg <= enumFin && enumFin <= srcCap then
                    if operKind == TokNotEqual then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "(!") else writer.putByte(outCap, outBuf, 40)
                    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "_eq(")
                    Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg, operPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
                    Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsStart, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "))")
                  else ()
                else
                  writer.putByte(outCap, outBuf, 40)
                  Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg, operPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                  Stigi.Emit.emitOperTok{writer}(outCap, outBuf, operKind)
                  Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsStart, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                  writer.putByte(outCap, outBuf, 41)
              else
                writer.putByte(outCap, outBuf, 40)
                Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg, operPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                Stigi.Emit.emitOperTok{writer}(outCap, outBuf, operKind)
                Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsStart, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                writer.putByte(outCap, outBuf, 41)
            else ()
          case OperNone =>
            if firstKind == TokBang && beg + 1 < fin then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "(!")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg + 1, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
              writer.putByte(outCap, outBuf, 41)
            else if firstKind == TokMinus && beg + 1 < fin then
              val nextPos: U64 = beg + 1
              val Token(negKind, negBeg, negFin, _, _) = toks(nextPos)
              if negKind == TokInt && nextPos + 1 == fin && negFin <= srcCap then
                Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "(-INT64_C(")
                writer.putSlice(outCap, outBuf, srcCap, src, negBeg, negFin)
                Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "))")
              else
                Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "(-")
                Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, nextPos, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                writer.putByte(outCap, outBuf, 41)
            else if firstKind == TokInt && firstFin <= srcCap then
              val litLen: U64 = firstFin - firstBeg
              writer.putByte(outCap, outBuf, 40)
              writer.putSlice(outCap, outBuf, srcCap, src, firstBeg, firstFin)
              if litLen >= 10 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "ULL)")
              else writer.putByte(outCap, outBuf, 41)
            else if firstKind == TokTrue then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true") else if firstKind == TokFalse then Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, "false") else if firstKind == TokChar && firstFin <= srcCap then
              writer.putSlice(outCap, outBuf, srcCap, src, firstBeg, firstFin)
            else if firstKind == TokStr && firstFin <= srcCap then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 13, "((uint8_t[]){")
              writer.putSlice(outCap, outBuf, srcCap, src, firstBeg, firstFin)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "})")
            else if firstKind == TokLeftParen && beg + 1 < fin && lastKind == TokRightParen then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "((void)0)")
            else if firstKind == TokIdent || firstKind == TokUpper then
              Stigi.Emit.emitIdentOrCall{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, beg, fin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
            else Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
  else Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
