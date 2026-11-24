// prompt: work item for walking syntax scopes and type annotations while tracking local length binders.
enum Stigi.Store.VisitWork:
  case VisitNode(node: U64)
  case RestoreBinders(mark: U64) <lessOrEqual(mark, 256)>
