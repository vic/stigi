// prompt: search a name of the current unit through the namespaces of the unit that holds it.
pub def Stigi.Names.searchName(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64) <below(from, Stigi.Limit.maxUnits), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.UnitLookup =
  image match
    case Image(pool, entries, count, _) =>
      if from < count then
        entries(from) match
          case Entry(fromFirst, fromEnd, _, _) => Stigi.Names.searchFrom(image, tables, from, first, end, Stigi.Names.namespaceEnd(pool, fromFirst, fromEnd, fromEnd - fromFirst + 1), fromEnd - fromFirst + 1)
      else UnitAbsent
