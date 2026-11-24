// prompt: whether a call starts its argument list with one leading core argument, the left side of a pipe.
enum Stigi.Elaborate.Leading:
  case WithoutLeading
  case LeadingArgument(at: U64)
