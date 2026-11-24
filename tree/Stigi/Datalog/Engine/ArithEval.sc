// prompt: represent the outcome of evaluating one checked 128-bit arithmetic literal.
enum Stigi.Datalog.Engine.ArithEval:
  case ArithVal(value: Stigi.Datalog.Store.Cell)
  case ArithErr
