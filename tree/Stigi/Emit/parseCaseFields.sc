// prompt: parse parenthesized fields of one enum case into flat columnar arrays.
def Stigi.Emit.parseCaseFields {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, openParenTok: U64, fieldCap: U64, fieldNameStart: Array[U64](fieldCap), fieldNameEnd: Array[U64](fieldCap), fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(openParenTok, tokCount), below(srcCap, 0x100000000), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096)>: U64 <lessOrEqual(fieldCount, result), lessOrEqual(result, fieldCap)> =
  val closeTok: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, openParenTok, 40, 41)
  var cursor: U64 = openParenTok + 1
  var curFields: U64 = fieldCount
  var stepBudget: U64 = 0
  while cursor < closeTok && stepBudget < tokCount do
    @<lessOrEqual(cursor, tokCount), lessOrEqual(fieldCount, curFields), lessOrEqual(curFields, fieldCap), lessOrEqual(stepBudget, tokCount), decreases(tokCount - stepBudget)>
    val colonTok: U64 = cursor + 1
    val typePos: U64 = cursor + 2
    if typePos < closeTok && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, colonTok, 1, ":") then
      if curFields < fieldCap then
        fieldNameStart(curFields) := tokStart(cursor)
        fieldNameEnd(curFields) := tokEnd(cursor)
        fieldTypeStart(curFields) := tokStart(typePos)
        fieldTypeEnd(curFields) := tokEnd(typePos)
        curFields = curFields + 1
      else
        ()
      var nextPos: U64 = typePos + 1
      if nextPos < closeTok && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, "[") then
        nextPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextPos, 91, 93)
      else
        ()
      if nextPos < closeTok && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, "(") then
        nextPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextPos, 40, 41)
      else
        ()
      cursor = if nextPos < closeTok && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, ",") then nextPos + 1 else nextPos
    else
      cursor = cursor + 1
    stepBudget = stepBudget + 1
  curFields
