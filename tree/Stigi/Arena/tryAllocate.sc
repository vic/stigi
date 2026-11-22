// prompt: bump-allocate an aligned block from an Arena chunk without overflow or crash.
pub def Stigi.Arena.tryAllocate(capacity: U64, used: U64, requestBytes: U64) <lessOrEqual(used, capacity), below(capacity, 0x100000000), below(requestBytes, 0x100000000)>: Stigi.Arena.AllocOutcome =
  val alignBytes: U64 = if requestBytes >= 64 then 64 else 8
  val alignedOffset: U64 = Stigi.Types.alignForward(used, alignBytes)
  val alignedSize: U64 = Stigi.Arena.alignBlock(requestBytes)
  if alignedOffset <= capacity then
    val remaining: U64 = capacity - alignedOffset
    if alignedSize <= remaining then
      Allocated(alignedOffset, alignedOffset + alignedSize)
    else
      ArenaExhausted
  else
    ArenaExhausted
