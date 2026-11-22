// prompt: prove every monotonic write into the C output buffer stays bounded by output capacity.
pub lemma def Stigi.Emit.outputOffsetBounded(writeOffset: U64, nextOffset: U64, outCap: U64) <lessOrEqual(writeOffset, nextOffset), lessOrEqual(nextOffset, outCap)>: <lessOrEqual(writeOffset, outCap)> =
  ()
