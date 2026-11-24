// prompt: lower the low link of a node on the stack to a bound when the bound is lower.
def Stigi.Names.lowerLink {Stigi.Mut} (nodeCap: Stigi.UnitCap, visits: Array[Stigi.Names.Visit](nodeCap), node: U64, bound: U64): Unit =
  if node < nodeCap then
    visits(node) match
      case OnStack(disc, low) => if bound < low then visits(node) := OnStack(disc, bound) else ()
      case Unseen | Placed(_) => ()
  else ()
