// prompt: prove every monotonic Arena bump cursor stays within chunk capacity.
pub lemma def Stigi.Arena.usedWithinCapacity(used: U64, nextUsed: U64, capacity: U64) <lessOrEqual(used, nextUsed), lessOrEqual(nextUsed, capacity)>: <lessOrEqual(used, capacity)> =
  ()
