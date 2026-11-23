// prompt: verify column-zero prompt header and absence of banned words.
pub def Stigi.TenetGate.checkSource(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64) <lessOrEqual(srcLen, srcCap)>: Stigi.TenetGate.GateOutcome =
  if srcLen >= 10 && Stigi.Lex.sliceEquals(srcCap, src, 0, 10, 10, "// prompt:") then Stigi.TenetGate.scanWords(srcCap, src, srcLen, 0, srcLen) else GateMissingPrompt
