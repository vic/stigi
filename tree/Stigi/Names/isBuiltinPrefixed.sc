// prompt: whether a text names a code case after Stigi.Code or an identifier type after Stigi.Id.
def Stigi.Names.isBuiltinPrefixed(tables: Stigi.Syntax.Tables, first: U64, end: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Bool =
  if Stigi.Names.prefixedIn(tables, first, end, 11, Stigi.Names.codePrefix, 0, 1570, Stigi.Names.codeCaseText, 1571, 12) then true else Stigi.Names.prefixedIn(tables, first, end, 9, Stigi.Names.identifierPrefix, 0, 156, Stigi.Names.identifierTypeText, 157, 10)
