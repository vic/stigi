// prompt: the namepath search of a name: try the name behind each namespace prefix of the current unit, nearest first, then the bare name.
def Stigi.Names.searchFrom(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64, prefixEnd: U64, remaining: U64) <below(0, Stigi.Limit.maxUnitBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Lookup =
  image match
    case Image(pool, entries, count, _) =>
      if remaining < 1 || count <= from then Missing else
        entries(from) match
          case Entry(fromFirst, _, _, _) =>
            if prefixEnd <= fromFirst then Stigi.Names.findName(image, tables, Bare(first, end)) else
              if prefixEnd <= Stigi.Limit.maxIndexBytes then
                Stigi.Names.findName(image, tables, Prefixed(fromFirst, prefixEnd, first, end)) match
                  case Found(slot) => Found(slot)
                  case Missing => Stigi.Names.searchFrom(image, tables, from, first, end, Stigi.Names.namespaceEnd(pool, fromFirst, prefixEnd, prefixEnd - fromFirst + 1), remaining - 1)
              else Missing
