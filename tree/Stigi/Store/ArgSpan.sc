// prompt: one NUL-terminated command line argument span and the position after it, or no argument left.
enum Stigi.Store.ArgSpan:
  case ArgAt(argStart: U64, argEnd: U64, nextPos: U64) <lessOrEqual(argStart, argEnd)>
  case ArgsEnd
