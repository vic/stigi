// prompt: why printing the syntax table of one unit stopped.
enum Stigi.Syntax.Print.Fault:
  case MalformedNode(slot: U64)
  case PrintTooDeep
