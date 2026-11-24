// prompt: the namepath search of a name kept in the member pool: try the name behind each namespace prefix of the searching unit, nearest first, then the bare name.
def Stigi.Names.searchPoolFrom(image: Stigi.Names.Image, members: Stigi.Names.Members, from: U64, first: U64, end: U64, prefixEnd: U64, remaining: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes), decreases(remaining)>: Stigi.Names.UnitLookup =
  image match
    case Image(pool, entries, count, _) =>
      members match
        case Members(memberPool, _, _, _, _) =>
          if remaining < 1 || count <= from then UnitAbsent else
            entries(from) match
              case Entry(fromFirst, _, _, _) =>
                if prefixEnd <= fromFirst then Stigi.Names.poolBareFind(image, memberPool, first, end) else
                  if prefixEnd <= Stigi.Limit.maxIndexBytes then
                    Stigi.Names.poolPrefixedFind(image, memberPool, fromFirst, prefixEnd, first, end) match
                      case UnitFound(slot) => UnitFound(slot)
                      case UnitAbsent => Stigi.Names.searchPoolFrom(image, members, from, first, end, Stigi.Names.namespaceEnd(pool, fromFirst, prefixEnd, prefixEnd - fromFirst + 1), remaining - 1)
                  else UnitAbsent
