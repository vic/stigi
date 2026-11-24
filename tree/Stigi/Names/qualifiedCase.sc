// prompt: resolve a qualified enum case: the text before the last dot names an enum unit and the text behind it is a case of that unit.
def Stigi.Names.qualifiedCase(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64) <below(0, Stigi.Limit.maxUnitBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Resolution =
  val Tables(source, _, _, _, _, _) = tables
  val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
  if first < dotAt && dotAt < end && dotAt + 1 < end then
    Stigi.Names.searchName(image, tables, from, first, dotAt) match
      case Missing => Unresolved
      case Found(hit) =>
        image match
          case Image(_, entries, count, _) =>
            if hit < count then
              entries(hit) match
                case Entry(_, _, kind, _) => if kind is EnumKind then Stigi.Names.ownedCase(members, tables, hit, dotAt + 1, end, Stigi.Names.firstMember(members, tables, dotAt + 1, end), Stigi.Names.tableSlots) else Unresolved
            else Unresolved
  else Unresolved
