// prompt: the state of one copy of linear terms into length parts: still copying, the parts are full, a coefficient is not unsigned, or a binder is outside the unit.
enum Stigi.Types.Copy:
  case Copying
  case CopyFull
  case CopyNegative
  case CopyBadBinder
