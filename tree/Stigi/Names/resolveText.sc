// prompt: what a reference text names: a unit by the namepath search, else an enum case when allowed, else a builtin.
def Stigi.Names.resolveText(scope: Stigi.Names.NameScope, tables: Stigi.Syntax.Tables, from: U64, text: Stigi.Syntax.Text, allowCase: Bool) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Resolution =
  val Text(first, end) = text
  scope match
    case NameScope(image, members) =>
      Stigi.Names.searchName(image, tables, from, first, end) match
        case UnitFound(entry) => ToUnit(entry)
        case UnitAbsent =>
          val Tables(source, _, _, _, _, _) = tables
          val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
          val cased: Stigi.Names.Resolution = if allowCase then if first < dotAt && dotAt < end && dotAt + 1 < end then Stigi.Names.qualifiedCase(image, members, tables, from, first, end) else Stigi.Names.unqualifiedCase(image, members, tables, from, first, end) else Unresolved
          cased match
            case Unresolved => if Stigi.Names.isBuiltin(tables, first, end, 0, Stigi.Names.builtinTextBytes + 1) then ToBuiltin else Unresolved
            case ToUnit(_) | ToCase(_, _) | ToBuiltin | AmbiguousCase => cased
