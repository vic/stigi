// prompt: prove that any strict path length below maxLen stays within maxLen for NUL placement.
pub lemma def Stigi.Posix.pathLengthWithinLimit(pathLen: U64, maxLen: U64) <below(pathLen, maxLen)>: <lessOrEqual(pathLen, maxLen)> =
  ()
