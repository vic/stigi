// prompt: closed result of compile-time AST evaluation and constant folding.
enum Stigi.Eval.Value:
  case ValWord(num: U64)
  case ValFlag(flag: Bool)
  case ValDynamic
  case ValOverflow
