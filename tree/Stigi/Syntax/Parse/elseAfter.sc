// prompt: find the else keyword of a conditional on the same line, or at the start of the next line at the indent of its statement.
def Stigi.Syntax.Parse.elseAfter(tables: Stigi.Syntax.Tables, token: U64, indent: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.ElsePosition =
  if Stigi.Syntax.Parse.kindAt(tables, token) is TokElse then ElseFound(token) else
    val next: U64 = Stigi.Syntax.Parse.lineStart(tables, token, Stigi.Limit.maxUnitBytes)
    if Stigi.Syntax.Parse.kindAt(tables, token) is TokNewline && Stigi.Syntax.Parse.kindAt(tables, next) is TokElse && Stigi.Syntax.Parse.columnAt(tables, next) == indent then ElseFound(next) else ElseMissing
