// prompt: nesting depth, first argument token and argument count while scanning a parenthesized run.
enum Stigi.Parse.ArgState:
  case ArgState(depth: U64, firstArg: U64, argCount: U64)
