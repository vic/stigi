// prompt: the result of reading one parameter entry of a head: its name and declared type, or a malformed table.
enum Stigi.Names.ParamRead:
  case ParamBroken
  case ParamFound(name: Stigi.Syntax.Text, declared: Stigi.Syntax.NodeAt)
