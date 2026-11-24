// prompt: the head of a val declaration: its name, capabilities and type, and the progress after the equals sign.
enum Stigi.Syntax.Parse.ValDeclHead:
  case ValDeclHead(name: Stigi.Syntax.Text, caps: Stigi.Syntax.Span, declared: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress)
  case ValDeclHeadFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
