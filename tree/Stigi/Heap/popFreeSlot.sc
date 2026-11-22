// prompt: pop the most recently freed block index from a non-empty Heap reuse stack.
pub def Stigi.Heap.popFreeSlot(cap: U64, freeSlots: Array[U64](cap), count: U64) <below(0, count), lessOrEqual(count, cap)>: U64 =
  val topIndex: U64 = count - 1
  freeSlots(topIndex)
