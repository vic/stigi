// prompt: report the first banned word at or after a position of a source buffer.
def Stigi.TenetGate.scanWords(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, pos: U64, remaining: U64) <lessOrEqual(srcLen, srcCap), lessOrEqual(pos, srcLen), decreases(remaining)>: Stigi.TenetGate.GateOutcome =
  if pos < srcLen && remaining > 0 then
    if src(pos) is 'a'..'z' | 'A'..'Z' then
      var endPos: U64 = pos + 1
      while endPos < srcLen && src(endPos) is 'a'..'z' | 'A'..'Z' do
        @<lessOrEqual(pos, endPos), lessOrEqual(endPos, srcLen)>
        endPos = endPos + 1
      if Stigi.TenetGate.isBannedWord(srcCap, src, pos, endPos) then GateBannedWord(pos) else Stigi.TenetGate.scanWords(srcCap, src, srcLen, endPos, remaining - 1)
    else Stigi.TenetGate.scanWords(srcCap, src, srcLen, pos + 1, remaining - 1)
  else GateClean
