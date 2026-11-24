// prompt: whether a text of the current unit is the name of a builtin, scanning the builtin lines from an offset, then the prefixed code and identifier names.
def Stigi.Names.isBuiltin(tables: Stigi.Syntax.Tables, first: U64, end: U64, at: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 || Stigi.Names.builtinTextBytes <= at then Stigi.Names.isBuiltinPrefixed(tables, first, end) else
    if Stigi.Names.builtinLineIs(tables, first, end, Stigi.Names.builtinText, at, Stigi.Names.builtinTextBytes + 1) then true else Stigi.Names.isBuiltin(tables, first, end, Stigi.Names.nextLine(Stigi.Names.builtinText, at, Stigi.Names.builtinTextBytes + 1), remaining - 1)
