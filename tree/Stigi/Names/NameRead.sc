// prompt: what a name node of the syntax table holds: its text, or nothing when the slot is no name.
enum Stigi.Names.NameRead:
  case NameFound(text: Stigi.Syntax.Text)
  case Nameless
