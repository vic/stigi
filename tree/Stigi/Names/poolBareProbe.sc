// prompt: probe the unit name table of an image from a slot for a bare name kept in the member pool.
def Stigi.Names.poolBareProbe(image: Stigi.Names.Image, memberPool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64, slot: U64, remaining: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes), below(slot, Stigi.Names.tableSlots), decreases(remaining)>: Stigi.Names.UnitLookup =
  if remaining < 1 then UnitAbsent else
    image match
      case Image(pool, entries, count, slots) =>
        slots(slot) match
          case Vacant => UnitAbsent
          case Keyed(key) =>
            if key < count && Stigi.Names.poolBareMatches(pool, entries(key), memberPool, first, end) then UnitFound(key) else Stigi.Names.poolBareProbe(image, memberPool, first, end, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
