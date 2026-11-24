// prompt: probe the unit name table of an image from a slot for a name kept in the member pool behind a namespace prefix of the name pool.
def Stigi.Names.poolPrefixedProbe(image: Stigi.Names.Image, memberPool: Array[U8](Stigi.Limit.maxIndexBytes), prefix: Stigi.Names.PoolRun, first: U64, end: U64, slot: U64, remaining: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes), below(slot, Stigi.Names.tableSlots), decreases(remaining)>: Stigi.Names.UnitLookup =
  if remaining < 1 then UnitAbsent else
    image match
      case Image(pool, entries, count, slots) =>
        slots(slot) match
          case Vacant => UnitAbsent
          case Keyed(key) =>
            if key < count && Stigi.Names.poolPrefixedMatches(pool, entries(key), memberPool, prefix, first, end) then UnitFound(key) else Stigi.Names.poolPrefixedProbe(image, memberPool, prefix, first, end, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
