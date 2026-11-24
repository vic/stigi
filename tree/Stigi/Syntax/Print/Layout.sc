// prompt: how the items of a list are laid out: joined by a comma or a bar, each on its own line, stacked line by line, each after a space, or comma-joined inside delimiters always, only when non-empty, or after a space when non-empty.
enum Stigi.Syntax.Print.Layout:
  case JoinedByComma
  case JoinedByBar
  case EachOnLine
  case StackedLines
  case EachAfterSpace
  case InDelimiters(delimiter: Stigi.Syntax.Print.Delimiter)
  case WrappedIn(delimiter: Stigi.Syntax.Print.Delimiter)
  case SpacedIn(delimiter: Stigi.Syntax.Print.Delimiter)
