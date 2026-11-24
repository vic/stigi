// prompt: a parenthesized run closed with its first argument token and argument count, or left open.
enum Stigi.Parse.ArgScan:
  case ArgsClosed(firstArg: U64, argCount: U64)
  case ArgsOpen
