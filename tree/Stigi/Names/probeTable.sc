// prompt: probe an open-addressed linear-probing hash table using splitmix64 and closed Lookup outcome.
def Stigi.Names.probeTable(slotCap: Stigi.NodeCap, occupied: Array[Bool](slotCap), keys: Array[U64](slotCap), targetKey: U64) <below(0, slotCap)>: Stigi.Lookup =
  val startSlot: U64 = Stigi.Raw.mixSlot(targetKey, slotCap)
  var step: U64 = 0
  var outcome: Stigi.Lookup = Missing
  var finished: Bool = false
  while step < slotCap && !finished do
    @<lessOrEqual(step, slotCap)>
    val rawSlot: U64 = startSlot + step
    val slot: U64 = rawSlot % slotCap
    if !occupied(slot) then
      finished = true
    else if keys(slot) == targetKey then
      outcome = Found(slot)
      finished = true
    else ()
    step = step + 1
  outcome
