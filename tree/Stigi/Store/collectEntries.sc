// prompt: add the rows of every unit entry from an index on to the lanes, stopping at the first full lane.
def Stigi.Store.collectEntries {Stigi.Mut} (lanes: Stigi.Store.FactsLanes, image: Stigi.Names.Image, index: U64, remaining: U64) <decreases(remaining)>: Stigi.Store.Collected =
  val Image(pool, entries, count, _) = image
  if remaining < 1 || count <= index then LanesFilled(lanes) else
    entries(index) match
      case Entry(first, end, kind, _) => Stigi.Store.putEntryLanes(lanes, pool, first, end, kind) match
        case CollectStopped(fault) => CollectStopped(fault)
        case LanesFilled(next) => Stigi.Store.collectEntries(next, image, index + 1, remaining - 1)
