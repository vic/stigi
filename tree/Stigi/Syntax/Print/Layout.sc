// prompt: how the items of a list are laid out: joined by a comma or a bar, each on its own line, stacked line by line, or each after a space.
enum Stigi.Syntax.Print.Layout:
  case JoinedByComma
  case JoinedByBar
  case EachOnLine
  case StackedLines
  case EachAfterSpace
