// prompt: the namepath search of a rewritten name, the join of two runs of the unit text: try it behind each namespace prefix of the current unit, nearest first, then bare.
def Stigi.Names.searchAliasedFrom(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, targetFirst: U64, targetEnd: U64, tailFirst: U64, tailEnd: U64, prefixEnd: U64, remaining: U64) <lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(tailFirst, tailEnd), lessOrEqual(tailEnd, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Lookup =
  image match
    case Image(pool, entries, count, _) =>
      if remaining < 1 || count <= from then Missing else
        entries(from) match
          case Entry(fromFirst, _, _, _) =>
            if prefixEnd <= fromFirst then Stigi.Names.findName(image, tables, Joined(targetFirst, targetEnd, tailFirst, tailEnd)) else
              if prefixEnd <= Stigi.Limit.maxIndexBytes then
                Stigi.Names.findName(image, tables, PrefixedJoined(fromFirst, prefixEnd, targetFirst, targetEnd, tailFirst, tailEnd)) match
                  case Found(slot) => Found(slot)
                  case Missing => Stigi.Names.searchAliasedFrom(image, tables, from, targetFirst, targetEnd, tailFirst, tailEnd, Stigi.Names.namespaceEnd(pool, fromFirst, prefixEnd, prefixEnd - fromFirst + 1), remaining - 1)
              else Missing
