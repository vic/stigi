// prompt: resolve an unqualified enum case: prefer the one case in the namespace of the current unit, then the one visible case, and report more than one as ambiguous.
def Stigi.Names.unqualifiedCase(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64) <below(from, Stigi.Limit.maxUnits), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Resolution =
  Stigi.Names.tallyCases(NameScope(image, members), tables, from, Text(first, end), Stigi.Names.firstMember(members, tables, first, end), CaseTally(0, 0, 0, 0, 0, 0), Stigi.Names.tableSlots) match
    case CaseTally(nearOwner, nearMember, nearCount, farOwner, farMember, farCount) =>
      if nearCount == 1 then ToCase(nearOwner, nearMember) else
        if 1 < nearCount then AmbiguousCase else
          if farCount == 1 then ToCase(farOwner, farMember) else
            if 1 < farCount then AmbiguousCase else Unresolved
