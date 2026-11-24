// prompt: the outcome of parsing one syntax node: its index and the progress after it, or the fault and the token where the parse stopped.
pub enum Stigi.Syntax.Parse.Built:
  case Built(node: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress)
  case Failed(fault: Stigi.Syntax.Parse.Fault, token: U64)
