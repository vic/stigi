// prompt: pop the component stack down to its root, placing every popped node in one component.
def Stigi.Names.popComponent {Stigi.Mut} (nodeCap: Stigi.UnitCap, visits: Array[Stigi.Names.Visit](nodeCap), sccStack: Array[U64](nodeCap), root: U64, comp: U64, sccTop: U64) <lessOrEqual(sccTop, nodeCap), decreases(sccTop)>: U64 =
  if sccTop < 1 then sccTop else
    val member: U64 = sccStack(sccTop - 1)
    if member < nodeCap then visits(member) := Placed(comp) else ()
    if member == root then sccTop - 1 else Stigi.Names.popComponent(nodeCap, visits, sccStack, root, comp, sccTop - 1)
