// prompt: search a name kept in the member pool through the namespaces of the unit that holds it.
def Stigi.Names.searchPoolName(image: Stigi.Names.Image, members: Stigi.Names.Members, from: U64, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Stigi.Names.UnitLookup =
  image match
    case Image(pool, entries, count, _) =>
      if from < count then
        entries(from) match
          case Entry(fromFirst, fromEnd, _, _) => Stigi.Names.searchPoolFrom(image, members, from, first, end, Stigi.Names.namespaceEnd(pool, fromFirst, fromEnd, fromEnd - fromFirst + 1), fromEnd - fromFirst + 1)
      else UnitAbsent
