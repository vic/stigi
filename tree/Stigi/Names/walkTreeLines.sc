// prompt: walk every unit named on the index lines from a line position against the finished image, growing the edges, or say which unit failed and why.
def Stigi.Names.walkTreeLines {region: Stigi.Region, Stigi.Mut} (tree: Stigi.Names.TreeRead, image: Stigi.Names.Image, members: Stigi.Names.Members, edges: Stigi.Names.Edges, walk: Stigi.Store.IndexWalk, remaining: U64) <decreases(remaining)>: Stigi.Names.TreeWalkOutcome =
  val TreeRead(dirCap, dirBuf, dirStart, dirEnd, scratch, tables, indexLen) = tree
  val IndexWalk(linePos, unitCount) = walk
  if indexLen <= linePos then TreeWalked(edges) else if remaining < 1 then TreeWalkStopped(unitCount, 0, LinesExhausted) else
    val TreeScratch(_, indexBuf, _, sourceBuf, tasks, locals) = scratch
    val nameEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, linePos, true)
    val lineEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, nameEnd, false)
    val nextLine: U64 = if lineEnd < indexLen then lineEnd + 1 else lineEnd
    if linePos < nameEnd then
      if unitCount < Stigi.Limit.maxUnits then
        Stigi.Names.loadTreeUnit{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, linePos, nameEnd) match
          case UnitMissing => TreeWalkStopped(unitCount, 0, TreeUnreadable)
          case UnitRead(nameSize, size) =>
            Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, sourceBuf, size) match
              case ParseRejected(_, _) | LexRejected(_) => TreeWalkStopped(unitCount, nameSize, SourceRejected)
              case Parsed(root, _, _) =>
                Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edges, 0, 0, 0, WalkSound), image, members, tables, unitCount, root) match
                  case WalkFailed(why) => TreeWalkStopped(unitCount, nameSize, why)
                  case Walked(grown) => Stigi.Names.walkTreeLines{region}(tree, image, members, grown, IndexWalk(nextLine, unitCount + 1), remaining - 1)
      else TreeWalkStopped(unitCount, 0, ImageExhausted)
    else Stigi.Names.walkTreeLines{region}(tree, image, members, edges, IndexWalk(nextLine, unitCount), remaining - 1)
