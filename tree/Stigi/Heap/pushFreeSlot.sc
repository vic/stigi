// prompt: push a freed block index onto a Heap size-class LIFO reuse stack.
pub def Stigi.Heap.pushFreeSlot {Stigi.Mut} (cap: U64, freeSlots: Array[U64](cap), count: U64, blockIndex: U64) <lessOrEqual(count, cap), below(cap, 0x100000000)>: U64 <lessOrEqual(count, result), lessOrEqual(result, cap)> =
  if count < cap then
    freeSlots(count) := blockIndex
    count + 1
  else
    count
