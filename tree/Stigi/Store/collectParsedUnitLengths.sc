// prompt: parse an in-memory unit source buffer and collect its lengthTerm, linearForm, sameLength, and termOffset rows.
def Stigi.Store.collectParsedUnitLengths {region: Stigi.Region, Stigi.Mut} (scratch: Stigi.Store.LengthScratch, lanes: Stigi.Store.LengthLanes, image: Stigi.Names.Image, unitIdx: U64, srcCap: U64, src: Array[U8](srcCap)) <below(unitIdx, Stigi.Limit.maxUnits), lessOrEqual(srcCap, Stigi.Limit.maxUnitBytes)>: Stigi.Store.LengthCollect =
  scratch match
    case LengthScratch(_, _, synTables, _, _, _, _, _, _, _, _, _, _, _, _, _) =>
      Stigi.Syntax.Parse.parseUnit(synTables, srcCap, src, srcCap) match
        case ParseRejected(_, _) | LexRejected(_) => LengthsCollected(lanes)
        case Parsed(rootSyn, _, _) =>
          Stigi.Store.walkUnitLengths{region}(synTables, scratch, image, unitIdx, rootSyn, lanes)
