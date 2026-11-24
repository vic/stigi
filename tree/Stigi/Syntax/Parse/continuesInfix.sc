// prompt: whether a binary operator or an is test binding tighter than a floor follows the parsed operand.
def Stigi.Syntax.Parse.continuesInfix(tables: Stigi.Syntax.Tables, built: Stigi.Syntax.Parse.Built, floor: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  built match
    case Failed(_, _) => false
    case Built(_, progress) =>
      val Progress(token, _, _, _) = progress
      val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, token)
      if kind is TokIs then floor < Stigi.Syntax.Print.precedence(Equals) else
        Stigi.Syntax.Parse.infixOf(kind) match
          case Infix(operator) => floor < Stigi.Syntax.Print.precedence(operator)
          case Standalone => false
