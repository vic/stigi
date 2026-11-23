// prompt: emit local variable references, handler state fields, array indexing, capability calls, enum constructors, and user def calls.
def Stigi.Emit.emitUserRef {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, qualBeg: U64, qualFin: U64, capArgBeg: U64, capArgEnd: U64, hasCall: Bool, argCount: U64, argBegs: Array[U64](32), argFins: Array[U64](32), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, fuel: U64) <below(0, outCap), lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(qualBeg, qualFin), lessOrEqual(capArgBeg, capArgEnd), lessOrEqual(capArgEnd, endTok), lessOrEqual(argCount, 32), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 && startTok < endTok && qualFin <= srcCap then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
    val Token(_, headBeg, headFin, _, _) = toks(startTok)
    val isSingleIdent: Bool = qualBeg == headBeg && qualFin == headFin
    var foundLocal: Bool = false
    var locKind: Stigi.Emit.FieldKind = FieldLocal
    var locTypeBeg: U64 = 0
    var locTypeFin: U64 = 0
    var locIdx: U64 = localCount
    while locIdx > 0 && !foundLocal do
      @<lessOrEqual(locIdx, localCount), lessOrEqual(locTypeBeg, locTypeFin), lessOrEqual(locTypeFin, srcCap)>
      locIdx = locIdx - 1
      val FieldInfo(curKind, nameBeg, nameFin, typeBeg, typeFin, _, _) = locals(locIdx)
      if nameFin <= srcCap && headFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, headBeg, headFin, nameBeg, nameFin) then
        foundLocal = true
        locKind = curKind
        if typeFin <= srcCap then
          locTypeBeg = typeBeg
          locTypeFin = typeFin
        else ()
      else ()
    if foundLocal && locKind == FieldLocalCap && !isSingleIdent && hasCall then
      val mtdPos: U64 = startTok + 2
      if mtdPos < endTok && headFin <= srcCap then
        val Token(_, mtdBeg, mtdFin, _, _) = toks(mtdPos)
        if mtdFin <= srcCap then
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, locTypeBeg, locTypeFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "_proto_")
          writer.putSlice(outCap, outBuf, srcCap, src, mtdBeg, mtdFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "(v_")
          writer.putSlice(outCap, outBuf, srcCap, src, headBeg, headFin)
          var argIdx: U64 = 0
          while argIdx < argCount do
            @<lessOrEqual(argIdx, argCount)>
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
            val argBeg: U64 = argBegs(argIdx)
            val argFin: U64 = argFins(argIdx)
            if argBeg <= argFin && argFin <= endTok then
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argBeg, argFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
            else ()
            argIdx = argIdx + 1
          writer.putByte(outCap, outBuf, 41)
        else ()
      else ()
    else if foundLocal && isSingleIdent && headFin <= srcCap then
      if locKind == FieldStateVar then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 11, "_hstate->f_")
        writer.putSlice(outCap, outBuf, srcCap, src, headBeg, headFin)
      else
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_")
        writer.putSlice(outCap, outBuf, srcCap, src, headBeg, headFin)
      val argZeroBeg: U64 = argBegs(0)
      val argZeroFin: U64 = argFins(0)
      if hasCall && argCount >= 1 && argZeroBeg <= argZeroFin && argZeroFin <= endTok then
        writer.putByte(outCap, outBuf, 91)
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        writer.putByte(outCap, outBuf, 93)
      else ()
    else
      var foundSym: Bool = false
      var symIdx: U64 = 0
      var matchIdx: U64 = 0
      while symIdx < symCount && !foundSym do
        @<lessOrEqual(symIdx, symCount), lessOrEqual(matchIdx, symCount)>
        val SymEntry(symKind, _, symBeg, symFin, _, _, _, _, _, _) = syms(symIdx)
        val isCallable: Bool = symKind == SymCase || symKind == SymFunc || symKind == SymGenericFunc || symKind == SymExternBody || symKind == SymVal
        if isCallable && symFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, qualBeg, qualFin, symBeg, symFin) then
          foundSym = true
          matchIdx = symIdx
        else ()
        symIdx = symIdx + 1
      if foundSym && matchIdx < symCount then
        val SymEntry(matchKind, ownerIdx, caseBeg, caseFin, _, _, subStart, subCount, _, _) = syms(matchIdx)
        if matchKind == SymCase && ownerIdx < symCount && caseFin <= srcCap then
          val SymEntry(_, _, enumBeg, enumFin, _, _, _, _, _, _) = syms(ownerIdx)
          if enumFin <= srcCap then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "((")
            Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "){ .tag = ")
            Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
            writer.putByte(outCap, outBuf, 95)
            writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
            if subCount > 0 && subStart <= fieldCount && subCount <= fieldCount then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, ", .u = { .")
              writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, " = { ")
              var fldOff: U64 = 0
              while fldOff < subCount && fldOff < argCount do
                @<lessOrEqual(fldOff, subCount), lessOrEqual(fldOff, argCount)>
                if fldOff > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
                val fldPos: U64 = subStart + fldOff
                if fldPos < fieldCount then
                  val FieldInfo(_, fldBeg, fldFin, _, _, _, _) = fields(fldPos)
                  if fldFin <= srcCap then
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".f_")
                    writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
                    val argBeg: U64 = argBegs(fldOff)
                    val argFin: U64 = argFins(fldOff)
                    if argBeg <= argFin && argFin <= endTok then
                      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argBeg, argFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
                    else ()
                  else ()
                else ()
                fldOff = fldOff + 1
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " } }")
            else ()
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " })")
          else ()
        else if matchKind == SymVal then
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, qualBeg, qualFin)
        else
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, qualBeg, qualFin)
          val genArgBeg: U64 = argBegs(0)
          val genArgFin: U64 = argFins(0)
          if matchKind == SymGenericFunc && argCount >= 1 && genArgBeg <= genArgFin && genArgFin <= endTok && monoFin <= srcCap then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "__")
            val argInf: Stigi.Emit.InferredType = Stigi.Emit.inferType(srcCap, src, tokCap, toks, genArgBeg, genArgFin, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin)
            argInf match
              case InferBool => Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "Bool")
              case InferWord => Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "U64")
              case InferWordShort => Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "U32")
              case InferByte => Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "U8")
              case InferSignedLong => Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "I64")
              case InferSignedWord => Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "I32")
              case InferEnum(_, enumBeg, enumFin) =>
                if enumFin <= srcCap then
                  var dotPos: U64 = enumBeg
                  while dotPos < enumFin do
                    @<lessOrEqual(enumBeg, dotPos), lessOrEqual(dotPos, enumFin)>
                    val curByte: U8 = src(dotPos)
                    writer.putByte(outCap, outBuf, if curByte == 46 then 95 else curByte)
                    dotPos = dotPos + 1
                else ()
              case InferNamed(nameBeg, nameFin) =>
                if nameFin <= srcCap then writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin) else ()
              case InferUnknown => Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "U64")
          else ()
          writer.putByte(outCap, outBuf, 40)
          var emittedAny: Bool = false
          var capPos: U64 = capArgBeg
          while capPos < capArgEnd do
            @<lessOrEqual(capArgBeg, capPos), lessOrEqual(capPos, capArgEnd)>
            val Token(capKind, _, _, _, _) = toks(capPos)
            if capKind == TokComma then capPos = capPos + 1 else
              val TypeSpan(capNameBeg, capNameFin, _, _, nextCapPos) = Stigi.Emit.parseTypeSpan(tokCap, toks, capPos, capArgEnd)
              if capNameFin <= srcCap then
                val omitMarker: Bool = Stigi.Lex.sliceEquals(srcCap, src, capNameBeg, capNameFin, 6, "region") || Stigi.Lex.sliceEquals(srcCap, src, capNameBeg, capNameFin, 12, "Stigi.Region") || Stigi.Lex.sliceEquals(srcCap, src, capNameBeg, capNameFin, 9, "Stigi.Mut")
                if !omitMarker then
                  if emittedAny then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_")
                  writer.putSlice(outCap, outBuf, srcCap, src, capNameBeg, capNameFin)
                  emittedAny = true
                else ()
              else ()
              capPos = if nextCapPos > capPos && nextCapPos <= capArgEnd then nextCapPos else capPos + 1
          var argIdx: U64 = 0
          while argIdx < argCount do
            @<lessOrEqual(argIdx, argCount)>
            if emittedAny then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
            val argBeg: U64 = argBegs(argIdx)
            val argFin: U64 = argFins(argIdx)
            if argBeg <= argFin && argFin <= endTok then
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argBeg, argFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
            else ()
            emittedAny = true
            argIdx = argIdx + 1
          writer.putByte(outCap, outBuf, 41)
      else
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
  else Stigi.Emit.writeLit{writer}(outCap, outBuf, 1, "0")
