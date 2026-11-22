// prompt: parse a stored tree index buffer into flat columnar unit metadata arrays.
pub def Stigi.Store.parseIndex {Stigi.Mut} (indexCap: U64, indexBytes: Array[U8](indexCap), indexLen: U64, maxUnits: U64, nameStarts: Array[U64](maxUnits), nameEnds: Array[U64](maxUnits), kindCodes: Array[U8](maxUnits), digestLow: Array[U64](maxUnits), digestMid: Array[U64](maxUnits), digestHigh: Array[U64](maxUnits), digestTop: Array[U64](maxUnits)) <lessOrEqual(indexLen, indexCap), below(indexCap, 0x100000000), below(maxUnits, 4096)>: Stigi.Store.IndexOutcome =
  var cursor: U64 = 0
  var unitCount: U64 = 0
  var stepCount: U64 = 0
  var corrupt: Bool = false
  while cursor < indexLen && stepCount < indexLen && !corrupt do
    @<lessOrEqual(cursor, indexLen), lessOrEqual(stepCount, indexLen), lessOrEqual(unitCount, maxUnits), decreases(indexLen - stepCount)>
    val lineEnd: U64 = Stigi.Store.findByteInRange(indexCap, indexBytes, cursor, indexLen, 10)
    if lineEnd < indexLen then
      val tabOne: U64 = Stigi.Store.findByteInRange(indexCap, indexBytes, cursor, lineEnd, 9)
      if cursor < tabOne && tabOne < lineEnd then
        val digestStart: U64 = tabOne + 1
        val tabTwo: U64 = Stigi.Store.findByteInRange(indexCap, indexBytes, digestStart, lineEnd, 9)
        if tabTwo < lineEnd && Stigi.Store.isHexDigestSlice(indexCap, indexBytes, digestStart, tabTwo) then
          val kindStart: U64 = tabTwo + 1
          val kindVal: Stigi.Store.UnitKind = Stigi.Store.parseUnitKind(indexCap, indexBytes, kindStart, lineEnd)
          val kindByte: U8 = Stigi.Store.unitKindCode(kindVal)
          if kindByte != 0 && unitCount < maxUnits && digestStart + 48 < indexLen then
            nameStarts(unitCount) := cursor
            nameEnds(unitCount) := tabOne
            kindCodes(unitCount) := kindByte
            digestLow(unitCount) := Stigi.Store.decodeHexWord(indexCap, indexBytes, digestStart)
            digestMid(unitCount) := Stigi.Store.decodeHexWord(indexCap, indexBytes, digestStart + 16)
            digestHigh(unitCount) := Stigi.Store.decodeHexWord(indexCap, indexBytes, digestStart + 32)
            digestTop(unitCount) := Stigi.Store.decodeHexWord(indexCap, indexBytes, digestStart + 48)
            unitCount = unitCount + 1
            cursor = lineEnd + 1
          else
            corrupt = true
        else
          corrupt = true
      else
        corrupt = true
    else
      corrupt = true
    stepCount = stepCount + 1
  if corrupt then
    IndexCorrupt(unitCount + 1)
  else
    IndexParsed(unitCount)
