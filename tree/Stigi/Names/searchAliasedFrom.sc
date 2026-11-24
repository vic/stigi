// prompt: the namepath search of a rewritten name, the join of two runs of the unit text: try it behind each namespace prefix of the current unit, nearest first, then bare.
def Stigi.Names.searchAliasedFrom(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, target: Stigi.Syntax.Text, tail: Stigi.Syntax.Text, prefixEnd: U64, remaining: U64) <decreases(remaining)>: Stigi.Names.UnitLookup =
  val Text(targetFirst, targetEnd) = target
  val Text(tailFirst, tailEnd) = tail
  image match
    case Image(pool, entries, count, _) =>
      if remaining < 1 || count <= from then UnitAbsent else
        entries(from) match
          case Entry(fromFirst, _, _, _) =>
            if prefixEnd <= fromFirst then Stigi.Names.probeName(image, tables, Joined(targetFirst, targetEnd, tailFirst, tailEnd), Stigi.Names.candidateSlot(pool, tables, Joined(targetFirst, targetEnd, tailFirst, tailEnd)), Stigi.Names.tableSlots) else
              if prefixEnd <= Stigi.Limit.maxIndexBytes then
                Stigi.Names.probeName(image, tables, PrefixedJoined(fromFirst, prefixEnd, targetFirst, targetEnd, tailFirst, tailEnd), Stigi.Names.candidateSlot(pool, tables, PrefixedJoined(fromFirst, prefixEnd, targetFirst, targetEnd, tailFirst, tailEnd)), Stigi.Names.tableSlots) match
                  case UnitFound(slot) => UnitFound(slot)
                  case UnitAbsent => Stigi.Names.searchAliasedFrom(image, tables, from, target, tail, Stigi.Names.namespaceEnd(pool, fromFirst, prefixEnd, prefixEnd - fromFirst + 1), remaining - 1)
              else UnitAbsent
