// prompt: push a named match or conditional assignment target onto the 32-word targetBox stack.
def Stigi.Emit.pushTargetEntry {Stigi.Mut} (targetBox: Array[U64](32), nameBeg: U64, nameFin: U64, frameIdx: U64) <lessOrEqual(0, 1)>: Unit =
  val curBase: U64 = targetBox(0)
  val nextBase: U64 = if curBase == 0 then 1 else if curBase < 25 then curBase + 4 else 32
  if nextBase < 29 then
    val finSlot: U64 = nextBase + 1
    val frameSlot: U64 = nextBase + 2
    val prevSlot: U64 = nextBase + 3
    if frameSlot < 32 && prevSlot < 32 then
      targetBox(nextBase) := nameBeg
      targetBox(finSlot) := nameFin
      targetBox(frameSlot) := frameIdx
      targetBox(prevSlot) := curBase
      targetBox(0) := nextBase
    else
      ()
  else
    ()
