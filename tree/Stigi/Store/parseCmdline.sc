// prompt: parse NUL-separated /proc/self/cmdline bytes into a closed Stigi.Store.CliAction.
def Stigi.Store.parseCmdline(bufCap: Stigi.ByteCap, buf: Array[U8](bufCap), bufLen: U64) <lessOrEqual(bufLen, bufCap)>: Stigi.Store.CliAction =
  var pos: U64 = 0
  while pos < bufLen && buf(pos) != 0 do
    @<lessOrEqual(pos, bufLen)>
    pos = pos + 1
  if pos < bufLen then pos = pos + 1 else ()
  if pos >= bufLen then CliRunContracts else
    val argOneStart: U64 = pos
    while pos < bufLen && buf(pos) != 0 do
      @<lessOrEqual(argOneStart, pos), lessOrEqual(pos, bufLen)>
      pos = pos + 1
    val argOneEnd: U64 = pos
    if pos < bufLen then pos = pos + 1 else ()
    if pos >= bufLen then CliInvalid else
      val argTwoStart: U64 = pos
      while pos < bufLen && buf(pos) != 0 do
        @<lessOrEqual(argTwoStart, pos), lessOrEqual(pos, bufLen)>
        pos = pos + 1
      val argTwoEnd: U64 = pos
      if pos < bufLen then pos = pos + 1 else ()
      if Stigi.Lex.sliceEquals(bufCap, buf, argOneStart, argOneEnd, 7, "--check") then
        if pos >= bufLen && argTwoStart < argTwoEnd then CliCheckTree(argTwoStart, argTwoEnd) else CliInvalid
      else if Stigi.Lex.sliceEquals(bufCap, buf, argOneStart, argOneEnd, 6, "--load") then
        if pos >= bufLen then CliInvalid else
          val argThreeStart: U64 = pos
          while pos < bufLen && buf(pos) != 0 do
            @<lessOrEqual(argThreeStart, pos), lessOrEqual(pos, bufLen)>
            pos = pos + 1
          val argThreeEnd: U64 = pos
          if pos < bufLen then pos = pos + 1 else ()
          if pos >= bufLen then CliInvalid else
            val argFourStart: U64 = pos
            while pos < bufLen && buf(pos) != 0 do
              @<lessOrEqual(argFourStart, pos), lessOrEqual(pos, bufLen)>
              pos = pos + 1
            val argFourEnd: U64 = pos
            if pos < bufLen then pos = pos + 1 else ()
            val validEmitFlag: Bool = Stigi.Lex.sliceEquals(bufCap, buf, argThreeStart, argThreeEnd, 6, "--emit")
            if validEmitFlag && pos >= bufLen && argTwoStart < argTwoEnd && argFourStart < argFourEnd then
              CliLoadEmit(argTwoStart, argTwoEnd, argFourStart, argFourEnd)
            else CliInvalid
      else CliInvalid
