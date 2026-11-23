// prompt: law proving alignOffset never decreases a field offset.
pub lemma def Stigi.Types.Laws.alignMonotone(offset: U64, alignBytes: U64) <below(offset, Stigi.Limit.maxSourceBytes), below(0, alignBytes), lessOrEqual(alignBytes, 64)>: <below(offset, Stigi.Limit.maxSourceBytes)> =
  val aligned: U64 = Stigi.Types.alignOffset(offset, alignBytes)
  @<lessOrEqual(offset, aligned)>
