// prompt: the part of a statement before its body: a val, a var, a handle, an assignment or a value, or the fault.
enum Stigi.Syntax.Parse.Head:
  case ValHead(pattern: Stigi.Syntax.NodeAt, declared: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress)
  case VarHead(name: Stigi.Syntax.Text, invariant: Stigi.Syntax.NodeAt, declared: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress)
  case HandleHead(binder: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, call: Stigi.Syntax.Parse.CallHead)
  case AssignHead(name: Stigi.Syntax.Text, progress: Stigi.Syntax.Parse.Progress)
  case ValueHead(progress: Stigi.Syntax.Parse.Progress)
  case HeadFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
