// prompt: require the closing parenthesis after a parsed expression in parentheses.
def Stigi.Syntax.Parse.closeParen(tables: Stigi.Syntax.Tables, inner: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  inner match
    case Failed(fault, at) => Failed(fault, at)
    case Built(node, after) =>
      val Progress(closing, nodes, lists, pending) = after
      if Stigi.Limit.maxUnitBytes <= closing || !(Stigi.Syntax.Parse.kindAt(tables, closing) is TokRightParen) then Failed(UnexpectedToken, closing) else Built(node, Progress(closing + 1, nodes, lists, pending))
