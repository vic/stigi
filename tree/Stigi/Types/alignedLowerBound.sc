// prompt: law stating that non-zero alignment preserves lower bound transitivity.
pub lemma def Stigi.Types.alignedLowerBound(value: U64, aligned: U64, limit: U64) <lessOrEqual(value, aligned), below(aligned, limit)>: <below(value, limit)> =
  @<below(value, limit)>
