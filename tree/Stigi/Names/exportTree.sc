// prompt: export the names of a tree: index every unit named on the index into the image, then walk every unit for its edges, and count both, or say which unit and step failed.
pub def Stigi.Names.exportTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.Names.ExportOutcome =
  val Filled(pathBuf) = Array.tryFill[U8]{region}(Stigi.Limit.maxPathBytes, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(indexBuf) = Array.tryFill[U8]{region}(Stigi.Limit.maxIndexBytes, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(nameBuf) = Array.tryFill[U8]{region}(Stigi.Limit.maxUnitBytes, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(sourceBuf) = Array.tryFill[U8]{region}(Stigi.Limit.maxUnitBytes, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(tasks) = Array.tryFill[Stigi.Names.Task]{region}(Stigi.Names.maxScope, Restore(0)) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(locals) = Array.tryFill[Stigi.Names.Local]{region}(Stigi.Names.maxScope, Local(Text(0, 0), WithoutCapability)) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(pool) = Array.tryFill[U8]{region}(Stigi.Limit.maxIndexBytes, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{region}(Stigi.Names.tableSlots, Vacant) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(memberPool) = Array.tryFill[U8]{region}(Stigi.Limit.maxIndexBytes, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(table) = Array.tryFill[Stigi.Names.Member]{region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(memberSlots) = Array.tryFill[Stigi.Names.KeySlot]{region}(Stigi.Names.tableSlots, Vacant) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(sources) = Array.tryFill[U64]{region}(Stigi.Names.maxEdges, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(targets) = Array.tryFill[U64]{region}(Stigi.Names.maxEdges, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Filled(stamps) = Array.tryFill[U64]{region}(Stigi.Limit.maxUnits, 0) else
    case FillFailed => ExportFailed(BufferUnavailable)
  val Allocated(tables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => ExportFailed(BufferUnavailable)
  val Found(indexPathLen) = Stigi.Store.buildIndexPath(dirCap, dirBuf, dirStart, dirEnd, pathBuf) else
    case Missing => ExportFailed(TreeUnreadable)
  if indexPathLen < Stigi.Limit.maxPathBytes then
    val LoadedBytes(indexLen) = Stigi.Posix.readFileBytes{region}(indexPathLen, pathBuf, Stigi.Limit.maxIndexBytes, indexBuf, 0) else
      case LoadFailed => ExportFailed(TreeUnreadable)
      case LoadOverflow => ExportFailed(TreeUnreadable)
    if indexLen <= Stigi.Limit.maxIndexBytes then
      val scratch: Stigi.Names.TreeScratch = TreeScratch(pathBuf, indexBuf, nameBuf, sourceBuf, tasks, locals)
      val tree: Stigi.Names.TreeRead = TreeRead(dirCap, dirBuf, dirStart, dirEnd, scratch, tables, indexLen)
      Stigi.Names.indexTreeLines{region}(tree, Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), IndexWalk(0, 0), 0, indexLen + 1) match
        case TreeIndexStopped(entry, nameSize, why) => ExportFailedAt(entry, IndexStep, why, nameBuf, nameSize)
        case TreeIndexed(image, members, _) =>
          Stigi.Names.walkTreeLines{region}(tree, image, members, Edges(sources, targets, 0, stamps), IndexWalk(0, 0), indexLen + 1) match
            case TreeWalkStopped(entry, nameSize, why) => ExportFailedAt(entry, WalkStep, why, nameBuf, nameSize)
            case TreeWalked(edges) =>
              val Image(_, _, units, _) = image
              val Edges(_, _, found, _) = edges
              Exported(units, found, image, edges)
    else ExportFailed(TreeUnreadable)
  else ExportFailed(TreeUnreadable)
