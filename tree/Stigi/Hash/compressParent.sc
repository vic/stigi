// prompt: compress two eight-word child chaining values into one parent chaining value.
def Stigi.Hash.compressParent {Stigi.Mut} (leftChain: Array[U32](8), rightChain: Array[U32](8), isRoot: Bool, initChain: Array[U32](8), message: Array[U32](16), scratch: Array[U32](16), state: Array[U32](16), destChain: Array[U32](8)) <lessOrEqual(0, 1)>: Unit =
  var lane: U64 = 0
  var upperSlot: U64 = 8
  while lane < 8 && upperSlot < 16 do
    @<lessOrEqual(lane, 8), lessOrEqual(upperSlot, 16), decreases(8 - lane)>
    initChain(lane) := Stigi.Hash.initWord(lane)
    message(lane) := leftChain(lane)
    message(upperSlot) := rightChain(lane)
    lane = lane + 1
    upperSlot = upperSlot + 1
  val flags: U32 = if isRoot then 12 else 4
  Stigi.Hash.compressBlock(initChain, message, scratch, state, 0, 0, 64, flags)
  var outLane: U64 = 0
  while outLane < 8 do
    @<lessOrEqual(outLane, 8), decreases(8 - outLane)>
    destChain(outLane) := state(outLane)
    outLane = outLane + 1
