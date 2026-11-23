// prompt: register all cases and payload fields of one enum unit into flat SoA tables.
pub def Stigi.Emit.registerEnumUnit {Stigi.Mut} (unitIdx: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldNameStart: Array[U64](fieldCap), fieldNameEnd: Array[U64](fieldCap), fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(srcCap, 0x100000000), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096)>: Stigi.Emit.RegCounts =
  var cursor: U64 = 0
  var curCases: U64 = caseCount
  var curFields: U64 = fieldCount
  var stepBudget: U64 = 0
  while cursor < tokCount && stepBudget < tokCount do
    @<lessOrEqual(cursor, tokCount), lessOrEqual(caseCount, curCases), lessOrEqual(curCases, caseCap), lessOrEqual(fieldCount, curFields), lessOrEqual(curFields, fieldCap), lessOrEqual(stepBudget, tokCount), decreases(tokCount - stepBudget)>
    val nameTok: U64 = cursor + 1
    if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "case") && nameTok < tokCount then
      val fieldStart: U64 = curFields
      val afterName: U64 = nameTok + 1
      if afterName < tokCount && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, afterName, 1, "(") then
        val updatedFields: U64 = Stigi.Emit.parseCaseFields(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, afterName, fieldCap, fieldNameStart, fieldNameEnd, fieldTypeStart, fieldTypeEnd, curFields)
        val addedFields: U64 = updatedFields - fieldStart
        if curCases < caseCap then
          caseEnumUnit(curCases) := unitIdx
          caseNameStart(curCases) := tokStart(nameTok)
          caseNameEnd(curCases) := tokEnd(nameTok)
          caseFieldBase(curCases) := fieldStart
          caseFieldCount(curCases) := addedFields
          curCases = curCases + 1
        else
          ()
        curFields = updatedFields
        cursor = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, afterName, 40, 41)
      else
        if curCases < caseCap then
          caseEnumUnit(curCases) := unitIdx
          caseNameStart(curCases) := tokStart(nameTok)
          caseNameEnd(curCases) := tokEnd(nameTok)
          caseFieldBase(curCases) := fieldStart
          caseFieldCount(curCases) := 0
          curCases = curCases + 1
        else
          ()
        cursor = afterName
    else
      cursor = cursor + 1
    stepBudget = stepBudget + 1
  EnumTotals(curCases, curFields)
