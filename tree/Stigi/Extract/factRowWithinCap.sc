// prompt: prove every extracted fact row index strictly below capacity stays within table capacity.
pub lemma def Stigi.Extract.factRowWithinCap(factCount: U64, factCap: U64) <below(factCount, factCap)>: <lessOrEqual(factCount, factCap)> =
  ()
