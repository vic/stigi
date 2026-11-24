// prompt: finish a node whose edges are all scanned: pass its low link to the parent frame and pop its component when it is a root.
def Stigi.Names.finishNode {Stigi.Mut} (nodeCap: Stigi.UnitCap, visits: Array[Stigi.Names.Visit](nodeCap), frames: Array[Stigi.Names.Frame](nodeCap), sccStack: Array[U64](nodeCap), node: U64, popped: Stigi.Names.Walk) <below(node, nodeCap)>: Stigi.Names.Walk =
  val Walk(clock, comps, sccTop, dfsTop) = popped
  if nodeCap <= comps || nodeCap < sccTop then popped else
    visits(node) match
      case OnStack(disc, low) =>
        if dfsTop < 1 || nodeCap < dfsTop then () else
          val Frame(parent, _) = frames(dfsTop - 1)
          Stigi.Names.lowerLink(nodeCap, visits, parent, low)
        if low == disc then Walk(clock, comps + 1, Stigi.Names.popComponent(nodeCap, visits, sccStack, node, comps, sccTop), dfsTop) else popped
      case Unseen | Placed(_) => popped
