// prompt: the state of gathering the items of a list: more items follow, the list is complete, or it failed.
enum Stigi.Syntax.Parse.Gather:
  case Gathering(progress: Stigi.Syntax.Parse.Progress)
  case Gathered(progress: Stigi.Syntax.Parse.Progress)
  case GatherFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
