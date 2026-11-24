// prompt: probe the unit name table of an image from a slot for a candidate name.
def Stigi.Names.probeName(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, candidate: Stigi.Names.Candidate, slot: U64, remaining: U64) <below(slot, Stigi.Names.tableSlots), decreases(remaining)>: Stigi.Lookup =
  if remaining < 1 then Missing else
    image match
      case Image(pool, entries, count, slots) =>
        slots(slot) match
          case Vacant => Missing
          case Keyed(key) =>
            if key < count && Stigi.Names.entryMatches(pool, entries(key), tables, candidate) then Found(key) else Stigi.Names.probeName(image, tables, candidate, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
