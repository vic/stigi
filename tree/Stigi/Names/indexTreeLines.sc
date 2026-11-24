// prompt: index every unit named on the index lines from a line position, growing the image and the name pool, or say which unit failed and why.
def Stigi.Names.indexTreeLines {region: Stigi.Region, Stigi.Mut} (tree: Stigi.Names.TreeRead, image: Stigi.Names.Image, members: Stigi.Names.Members, walk: Stigi.Store.IndexWalk, poolEnd: U64, remaining: U64) <lessOrEqual(poolEnd, Stigi.Limit.maxIndexBytes), decreases(remaining)>: Stigi.Names.TreeIndexOutcome =
  val TreeRead(dirCap, dirBuf, dirStart, dirEnd, scratch, tables, indexLen) = tree
  val IndexWalk(linePos, unitCount) = walk
  if indexLen <= linePos then TreeIndexed(image, members, poolEnd) else if remaining < 1 then TreeIndexStopped(unitCount, 0, LinesExhausted) else
    val TreeScratch(_, indexBuf, nameBuf, sourceBuf, _, _) = scratch
    val nameEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, linePos, true)
    val lineEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, nameEnd, false)
    val nextLine: U64 = if lineEnd < indexLen then lineEnd + 1 else lineEnd
    if linePos < nameEnd then
      if unitCount < Stigi.Limit.maxUnits then
        Stigi.Names.loadTreeUnit{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, linePos, nameEnd) match
          case UnitMissing => TreeIndexStopped(unitCount, 0, TreeUnreadable)
          case UnitRead(nameSize, size) =>
            Stigi.Names.indexUnit(image, members, tables, poolEnd, UnitSource(nameBuf, nameSize, sourceBuf, size)) match
              case IndexFailed(why) => TreeIndexStopped(unitCount, nameSize, why)
              case Indexed(grown, recorded, newEnd) => Stigi.Names.indexTreeLines{region}(tree, grown, recorded, IndexWalk(nextLine, unitCount + 1), newEnd, remaining - 1)
      else TreeIndexStopped(unitCount, 0, ImageExhausted)
    else Stigi.Names.indexTreeLines{region}(tree, image, members, IndexWalk(nextLine, unitCount), poolEnd, remaining - 1)
