// prompt: search a rewritten name, the join of two runs of the unit text, through the namespaces of the unit that holds it.
def Stigi.Names.searchAliased(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, target: Stigi.Syntax.Text, tail: Stigi.Syntax.Text) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.UnitLookup =
  image match
    case Image(pool, entries, count, _) =>
      if from < count then
        entries(from) match
          case Entry(fromFirst, fromEnd, _, _) => Stigi.Names.searchAliasedFrom(image, tables, from, target, tail, Stigi.Names.namespaceEnd(pool, fromFirst, fromEnd, fromEnd - fromFirst + 1), fromEnd - fromFirst + 1)
      else UnitAbsent
