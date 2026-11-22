// prompt: law stating that monotonic LEB128 cursor advancement preserves capacity transitivity.
pub lemma def Stigi.Identity.lebOffsetMonotone(startOffset: U64, nextOffset: U64, capacity: U64) <lessOrEqual(startOffset, nextOffset), lessOrEqual(nextOffset, capacity)>: <lessOrEqual(startOffset, capacity)> =
  @<lessOrEqual(startOffset, capacity)>
