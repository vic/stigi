// prompt: move past the closing token of a parsed list.
def Stigi.Syntax.Parse.passCloser(listed: Stigi.Syntax.Parse.Listed) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Listed =
  listed match
    case ListFailed(fault, at) => ListFailed(fault, at)
    case Listed(items, after) =>
      val Progress(closing, nodes, lists, pending) = after
      if Stigi.Limit.maxUnitBytes <= closing then ListFailed(UnexpectedToken, closing) else Listed(items, Progress(closing + 1, nodes, lists, pending))
