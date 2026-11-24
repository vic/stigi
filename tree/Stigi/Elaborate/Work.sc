// prompt: one elaboration work item: a syntax node to enter, which pushes its children, or to leave, which emits its core node.
enum Stigi.Elaborate.Work:
  case Enter(node: U64) <below(node, Stigi.Limit.maxUnitNodes)>
  case Leave(node: U64) <below(node, Stigi.Limit.maxUnitNodes)>
