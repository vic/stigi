// prompt: one local binder of a unit body: its name text and the capability it stands for.
enum Stigi.Names.Local:
  case Local(text: Stigi.Syntax.Text, binding: Stigi.Names.CapabilityBinding)
