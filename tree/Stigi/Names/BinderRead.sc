// prompt: what a binder slot of a capability entry holds: a name, nothing for a bare capability reference, or a malformed slot.
enum Stigi.Names.BinderRead:
  case BinderNamed(text: Stigi.Syntax.Text)
  case BinderUnnamed
  case BinderBroken
