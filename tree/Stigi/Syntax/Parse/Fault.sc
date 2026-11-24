// prompt: why a parse of one unit stopped.
pub enum Stigi.Syntax.Parse.Fault:
  case UnexpectedToken
  case TooDeep
  case UnitTooLarge
  case ChildAfterParent
