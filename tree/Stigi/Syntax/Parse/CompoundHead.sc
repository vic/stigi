// prompt: the head of a declaration that holds member lines, up to the colon before its members.
enum Stigi.Syntax.Parse.CompoundHead:
  case CompoundHead(name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, capParams: Stigi.Syntax.Span, valueGroup: Bool, valueParams: Stigi.Syntax.Span, target: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress)
  case CompoundHeadFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
