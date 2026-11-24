// prompt: probe the unit name table of an image from a slot for a candidate name.
def Stigi.Names.probeName(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, candidate: Stigi.Names.Candidate, slot: U64, remaining: U64) <below(slot, Stigi.Names.tableSlots), decreases(remaining)>: Stigi.Names.UnitLookup =
  if remaining < 1 then UnitAbsent else
    image match
      case Image(pool, entries, count, slots) =>
        slots(slot) match
          case Vacant => UnitAbsent
          case Keyed(key) =>
            if key < count then
              entries(key) match
                case Entry(entryFirst, entryEnd, _, _) =>
                  if Stigi.Names.entryMatches(pool, entryFirst, entryEnd, tables, candidate) then UnitFound(key) else Stigi.Names.probeName(image, tables, candidate, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
            else Stigi.Names.probeName(image, tables, candidate, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
