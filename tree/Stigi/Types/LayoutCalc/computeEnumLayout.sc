// prompt: compute exact C-ABI enum layout with nullable-pointer and tail-padding tag optimizations.
pub def Stigi.Types.LayoutCalc.computeEnumLayout {region: Stigi.Region, Stigi.Mut} (caseCap: U64, cases: Array[Stigi.Types.LayoutCalc.CaseSpec](caseCap), caseCount: U64, fieldCap: U64, fields: Array[Stigi.Types.LayoutCalc.FieldKind](fieldCap), fieldCount: U64) <below(0, caseCap), lessOrEqual(caseCount, caseCap), below(0, fieldCap), lessOrEqual(fieldCount, fieldCap)>: Stigi.Types.LayoutCalc.EnumLayout =
  val tagSize: U64 = if caseCount <= 256 then 1 else if caseCount <= 65536 then 2 else 4
  val isNullPtr: Bool = if caseCount == 2 then
    cases(0) match
      case CaseInfo(firstZero, countZero) =>
        cases(1) match
          case CaseInfo(firstOne, countOne) =>
            val zeroNull: Bool = if countZero == 0 && countOne == 1 && firstOne < fieldCount then Stigi.Types.LayoutCalc.isPointerField(fields(firstOne)) else false
            val oneNull: Bool = if countOne == 0 && countZero == 1 && firstZero < fieldCount then Stigi.Types.LayoutCalc.isPointerField(fields(firstZero)) else false
            zeroNull || oneNull
  else
    false
  if isNullPtr then
    EnumLayoutInfo(8, 8, 0, 0, true, false)
  else
    val zeroWord: U64 = 0
    Array.tryFill{region}(fieldCap, zeroWord) match
      case FillFailed => EnumLayoutOverflow
      case Filled(order) =>
        var unionAlign: U64 = 1
        var unionSize: U64 = 0
        var maxRawEnd: U64 = 0
        var hasAnyFields: Bool = false
        var overflowed: Bool = false
        var caseIdx: U64 = 0
        while caseIdx < caseCount && !overflowed do
          @<lessOrEqual(caseIdx, caseCount), below(0, unionAlign), lessOrEqual(unionAlign, 64)>
          val nextCase: U64 = caseIdx + 1
          val nextUnionAlign: U64 = cases(caseIdx) match
            case CaseInfo(firstField, fldCount) =>
              if fldCount > 0 && firstField < 65536 && fldCount < 65536 then
                val fieldEnd: U64 = firstField + fldCount
                if fieldEnd <= fieldCount then
                  var nonEmpty: U64 = 0
                  var scanPos: U64 = firstField
                  while scanPos < fieldEnd do
                    @<lessOrEqual(firstField, scanPos), lessOrEqual(scanPos, fieldEnd), lessOrEqual(nonEmpty, fieldCap)>
                    val nextScan: U64 = scanPos + 1
                    val fldSize: U64 = Stigi.Types.LayoutCalc.fieldSize(fields(scanPos))
                    val nextNonEmpty: U64 = if fldSize > 0 && nonEmpty < fieldCap then
                      val advNonEmpty: U64 = nonEmpty + 1
                      order(nonEmpty) := scanPos
                      advNonEmpty
                    else
                      nonEmpty
                    nonEmpty = nextNonEmpty
                    scanPos = nextScan
                  if nonEmpty >= 1 then
                    hasAnyFields = true
                    var sortIdx: U64 = 1
                    while sortIdx < nonEmpty do
                      @<lessOrEqual(sortIdx, nonEmpty)>
                      val nextSort: U64 = sortIdx + 1
                      val keySlot: U64 = order(sortIdx)
                      val keyAlign: U64 = if keySlot < fieldCap then Stigi.Types.LayoutCalc.fieldAlign(fields(keySlot)) else 1
                      var insPos: U64 = sortIdx
                      var shifting: Bool = true
                      while insPos > 0 && shifting do
                        @<below(insPos, nonEmpty)>
                        val prevPos: U64 = insPos - 1
                        val prevSlot: U64 = order(prevPos)
                        val prevAlign: U64 = if prevSlot < fieldCap then Stigi.Types.LayoutCalc.fieldAlign(fields(prevSlot)) else 1
                        if prevAlign < keyAlign then
                          order(insPos) := prevSlot
                          insPos = prevPos
                        else
                          shifting = false
                      order(insPos) := keySlot
                      sortIdx = nextSort
                    var caseAlign: U64 = 1
                    var rawEnd: U64 = 0
                    var packIdx: U64 = 0
                    while packIdx < nonEmpty && !overflowed do
                      @<lessOrEqual(packIdx, nonEmpty), below(0, caseAlign), lessOrEqual(caseAlign, 64)>
                      val nextPack: U64 = packIdx + 1
                      val fldSlot: U64 = order(packIdx)
                      val nextCaseAlign: U64 = if fldSlot < fieldCap then
                        val fldKind: Stigi.Types.LayoutCalc.FieldKind = fields(fldSlot)
                        val fldSize: U64 = Stigi.Types.LayoutCalc.fieldSize(fldKind)
                        val fldAlign: U64 = Stigi.Types.LayoutCalc.fieldAlign(fldKind)
                        if rawEnd < Stigi.Limit.maxSourceBytes then
                          val alignedOff: U64 = Stigi.Types.alignOffset(rawEnd, fldAlign)
                          if alignedOff < Stigi.Limit.maxSourceBytes && fldSize < Stigi.Limit.maxSourceBytes then
                            rawEnd = alignedOff + fldSize
                          else
                            overflowed = true
                        else
                          overflowed = true
                        if fldAlign > caseAlign then fldAlign else caseAlign
                      else
                        overflowed = true
                        caseAlign
                      caseAlign = nextCaseAlign
                      packIdx = nextPack
                    if !overflowed && rawEnd < Stigi.Limit.maxSourceBytes then
                      val caseSize: U64 = Stigi.Types.alignOffset(rawEnd, caseAlign)
                      if rawEnd > maxRawEnd then
                        maxRawEnd = rawEnd
                      else
                        ()
                      if caseSize > unionSize then
                        unionSize = caseSize
                      else
                        ()
                      if caseAlign > unionAlign then caseAlign else unionAlign
                    else
                      overflowed = true
                      unionAlign
                  else
                    unionAlign
                else
                  overflowed = true
                  unionAlign
              else
                unionAlign
          if nextUnionAlign > 0 && nextUnionAlign <= 64 then unionAlign = nextUnionAlign else overflowed = true
          caseIdx = nextCase
        if overflowed then
          EnumLayoutOverflow
        else if !hasAnyFields then
          EnumLayoutInfo(tagSize, tagSize, tagSize, 0, false, false)
        else
          val totalAlign: U64 = if unionAlign > tagSize then unionAlign else tagSize
          if unionSize < Stigi.Limit.maxSourceBytes && maxRawEnd < Stigi.Limit.maxSourceBytes then
            val normUnion: U64 = Stigi.Types.alignOffset(unionSize, unionAlign)
            if normUnion < Stigi.Limit.maxSourceBytes then
              val paddedUnion: U64 = Stigi.Types.alignOffset(normUnion, totalAlign)
              val candTagOff: U64 = Stigi.Types.alignOffset(maxRawEnd, tagSize)
              if maxRawEnd > 0 && candTagOff < Stigi.Limit.maxSourceBytes && candTagOff + tagSize <= paddedUnion then
                EnumLayoutInfo(paddedUnion, totalAlign, tagSize, candTagOff, false, true)
              else
                val appTagOff: U64 = Stigi.Types.alignOffset(normUnion, tagSize)
                if appTagOff < Stigi.Limit.maxSourceBytes then
                  val rawTotal: U64 = appTagOff + tagSize
                  if rawTotal < Stigi.Limit.maxSourceBytes then
                    val finalSize: U64 = Stigi.Types.alignOffset(rawTotal, totalAlign)
                    EnumLayoutInfo(finalSize, totalAlign, tagSize, appTagOff, false, false)
                  else
                    EnumLayoutOverflow
                else
                  EnumLayoutOverflow
            else
              EnumLayoutOverflow
          else
            EnumLayoutOverflow
