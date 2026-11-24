// prompt: probe an open-addressed linear-probing key table from a slot with closed Lookup outcome.
def Stigi.Names.probeTable(slotCap: Stigi.NodeCap, table: Array[Stigi.Names.KeySlot](slotCap), targetKey: U64, slot: U64, remaining: U64) <below(slot, slotCap), decreases(remaining)>: Stigi.Lookup =
  if remaining < 1 then Missing else
    table(slot) match
      case Vacant => Missing
      case Keyed(key) => if key == targetKey then Found(slot) else Stigi.Names.probeTable(slotCap, table, targetKey, (slot + 1) % slotCap, remaining - 1)
