// prompt: verify that a source buffer starts at column zero with a non-empty prompt line.
pub def Stigi.TenetGate.hasPromptLine(length: U64, source: Array[U8](length)) <lessOrEqual(0, length)>: Bool =
  if length > 11 then
    val headMatch: Bool = source(0) == 47 && source(1) == 47 && source(2) == 32 && source(3) == 112 && source(4) == 114 && source(5) == 111
    val tailMatch: Bool = source(6) == 109 && source(7) == 112 && source(8) == 116 && source(9) == 58 && source(10) == 32 && source(11) != 10
    headMatch && tailMatch
  else
    false
