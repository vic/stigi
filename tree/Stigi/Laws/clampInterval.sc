// prompt: law proving clampWord stays inside its ordered closed interval.
pub lemma def Stigi.Laws.clampInterval(value: U64, lowBound: U64, highBound: U64) <lessOrEqual(lowBound, highBound)>: <lessOrEqual(lowBound, highBound)> =
  val clamped: U64 = Stigi.clampWord(value, lowBound, highBound)
  @<lessOrEqual(lowBound, clamped), lessOrEqual(clamped, highBound)>
