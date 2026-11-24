// prompt: the outcome of parsing a list of items: its span in the list table and the progress after it, or the fault.
enum Stigi.Syntax.Parse.Listed:
  case Listed(items: Stigi.Syntax.Span, progress: Stigi.Syntax.Parse.Progress)
  case ListFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
