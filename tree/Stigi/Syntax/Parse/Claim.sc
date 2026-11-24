// prompt: the next free entry of the node table claimed above up to three earlier child nodes, or why it cannot be claimed.
enum Stigi.Syntax.Parse.Claim:
  case Claimed(at: Stigi.Syntax.NodeAt, first: Stigi.Syntax.NodeAt, second: Stigi.Syntax.NodeAt, third: Stigi.Syntax.NodeAt) <below(first, at), below(second, at), below(third, at)>
  case Unclaimed(fault: Stigi.Syntax.Parse.Fault)
