// prompt: search a rewritten name, the join of two runs of the unit text, through the namespaces of the unit that holds it.
def Stigi.Names.searchAliased(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, targetFirst: U64, targetEnd: U64, tailFirst: U64, tailEnd: U64) <lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(tailFirst, tailEnd), lessOrEqual(tailEnd, Stigi.Limit.maxUnitBytes)>: Stigi.Lookup =
  image match
    case Image(pool, entries, count, _) =>
      if from < count then
        entries(from) match
          case Entry(fromFirst, fromEnd, _, _) => Stigi.Names.searchAliasedFrom(image, tables, from, targetFirst, targetEnd, tailFirst, tailEnd, Stigi.Names.namespaceEnd(pool, fromFirst, fromEnd, fromEnd - fromFirst + 1), fromEnd - fromFirst + 1)
      else Missing
