// prompt: verify column-zero prompt header, absence of banned words, and naming rules.
pub def Stigi.TenetGate.checkSource(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64) <lessOrEqual(srcLen, srcCap)>: Stigi.TenetGate.GateOutcome =
  val hasPrompt: Bool =
    if srcLen >= 10 then Stigi.Lex.sliceEquals(srcCap, src, 0, 10, 10, "// prompt:") else false
  if !hasPrompt then GateMissingPrompt else
    var pos: U64 = 0
    var outcome: Stigi.TenetGate.GateOutcome = GateClean
    var foundErr: Bool = false
    while pos < srcLen && !foundErr do
      @<lessOrEqual(pos, srcLen)>
      val curByte: U8 = src(pos)
      val isAlpha: Bool = curByte >= 65 && curByte <= 90 || curByte >= 97 && curByte <= 122
      if isAlpha then
        var endPos: U64 = pos + 1
        while endPos < srcLen && (src(endPos) >= 65 && src(endPos) <= 90 || src(endPos) >= 97 && src(endPos) <= 122) do
          @<lessOrEqual(pos, endPos), lessOrEqual(endPos, srcLen)>
          endPos = endPos + 1
        if Stigi.TenetGate.isBannedWord(srcCap, src, pos, endPos) then
          outcome = GateBannedWord(pos)
          foundErr = true
        else ()
        pos = endPos
      else
        pos = pos + 1
    outcome
