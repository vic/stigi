// prompt: add one unit to the names image, hashing its name from the pool into the unit name table.
def Stigi.Names.addEntry {Stigi.Mut} (image: Stigi.Names.Image, first: U64, end: U64, shape: Stigi.Names.UnitShape) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Stigi.Names.Added =
  shape match
    case Unshaped => UnitUnshaped
    case Shaped(kind, isPublic) =>
      image match
        case Image(pool, entries, count, slots) =>
          if Stigi.Limit.maxUnits <= count then ImageFull else
            entries(count) := Entry(first, end, kind, isPublic)
            if Stigi.Names.placeKey(slots, count, Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, first, end, false, Stigi.Names.hashSeed, end - first + 1) % Stigi.Names.tableSlots, Stigi.Names.tableSlots) then Added(Image(pool, entries, count + 1, slots)) else ImageFull
