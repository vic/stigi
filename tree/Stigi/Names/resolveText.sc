// prompt: what a reference text names: a unit by the namepath search, else an enum case when allowed, else a builtin.
def Stigi.Names.resolveText(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64, allowCase: Bool) <below(0, Stigi.Limit.maxUnitBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Resolution =
  Stigi.Names.searchName(image, tables, from, first, end) match
    case Found(entry) => ToUnit(entry)
    case Missing =>
      val Tables(source, _, _, _, _, _) = tables
      val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
      val cased: Stigi.Names.Resolution = if allowCase then if first < dotAt && dotAt < end && dotAt + 1 < end then Stigi.Names.qualifiedCase(image, members, tables, from, first, end) else Stigi.Names.unqualifiedCase(image, members, tables, from, first, end) else Unresolved
      cased match
        case Unresolved => if Stigi.Names.isBuiltin(tables, first, end, 0, 1172) then ToBuiltin else Unresolved
        case ToUnit(_) | ToCase(_, _) | ToBuiltin | AmbiguousCase => cased
