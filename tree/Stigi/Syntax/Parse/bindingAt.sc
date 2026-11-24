// prompt: the binding strength of the binary operator at a token, or zero.
def Stigi.Syntax.Parse.bindingAt(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: U64 =
  Stigi.Syntax.Parse.infixOf(Stigi.Syntax.Parse.kindAt(tables, token)) match
    case Infix(operator) => Stigi.Syntax.Print.precedence(operator)
    case Standalone => 0
