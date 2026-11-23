// prompt: emit C local bindings for parenthesized enum pattern fields and register them in the local table.
pub def Stigi.Emit.emitPatternBindings {region: Stigi.Region, Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, openParenTok: U64, closeParenTok: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, fieldBase: U64, fieldLen: U64, localTag: U64, isScrutinee: Bool, tempNum: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(openParenTok, closeParenTok), lessOrEqual(closeParenTok, tokCount), below(srcCap, 0x100000000), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), below(localCap, 256), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = writeOffset
  if openParenTok < closeParenTok && fieldLen <= fieldCount && fieldBase < fieldCount then
    var cursor: U64 = openParenTok + 1
    var fieldPos: U64 = fieldBase
    var fieldStep: U64 = 0
    var stepBudget: U64 = 0
    while cursor < closeParenTok && fieldStep < fieldLen && fieldPos < fieldCount && stepBudget < tokCount do
      @<lessOrEqual(cursor, closeParenTok), lessOrEqual(fieldStep, fieldLen), lessOrEqual(fieldStep, fieldCount), lessOrEqual(fieldPos, fieldCount), lessOrEqual(stepBudget, tokCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(tokCount - stepBudget)>
      if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, ",") || Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, ")") then
        cursor = cursor + 1
      else
        if !Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "_") then
          val bindBeg: U64 = tokStart(cursor)
          val bindFin: U64 = tokEnd(cursor)
          val typeBeg: U64 = fieldTypeStart(fieldPos)
          val typeFin: U64 = fieldTypeEnd(fieldPos)
          if bindBeg <= bindFin && bindFin <= srcCap && typeBeg <= typeFin && typeFin <= srcCap then
            pos = Stigi.Emit.emitTypeSlice(srcCap, srcBytes, typeBeg, typeFin, outCap, output, pos)
            pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
            pos = Stigi.Print.writeSlice(srcCap, srcBytes, bindBeg, bindFin, outCap, output, pos)
            pos = if isScrutinee then Stigi.Emit.writeLiteral(6, " = _sc", outCap, output, pos) else Stigi.Emit.writeLiteral(6, " = _ds", outCap, output, pos)
            pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
            pos = Stigi.Emit.writeLiteral(4, ".u.c", outCap, output, pos)
            pos = Stigi.Emit.writeUnsignedDecimal{region}(localTag, outCap, output, pos)
            pos = Stigi.Emit.writeLiteral(2, ".f", outCap, output, pos)
            pos = Stigi.Emit.writeUnsignedDecimal{region}(fieldStep, outCap, output, pos)
            pos = Stigi.Emit.writeLiteral(8, "; (void)", outCap, output, pos)
            pos = Stigi.Print.writeSlice(srcCap, srcBytes, bindBeg, bindFin, outCap, output, pos)
            pos = Stigi.Emit.writeLine(1, ";", outCap, output, pos)
            val curLoc: U64 = localCountBox(0)
            if curLoc < localCap then
              localNameStart(curLoc) := bindBeg
              localNameEnd(curLoc) := bindFin
              localTypeStart(curLoc) := typeBeg
              localTypeEnd(curLoc) := typeFin
              localIsArray(curLoc) := 0
              localCountBox(0) := curLoc + 1
            else
              ()
          else
            ()
        else
          ()
        fieldPos = fieldPos + 1
        fieldStep = fieldStep + 1
        cursor = cursor + 1
      stepBudget = stepBudget + 1
  else
    ()
  pos
