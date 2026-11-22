// prompt: decode one minimal unsigned LEB128 integer rejecting truncated, non-minimal, and overflowing forms.
pub def Stigi.Identity.decodeUnsignedLeb(capacity: U64, input: Array[U8](capacity), endOffset: U64, startOffset: U64) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, capacity), below(capacity, 0x100000000)>: Stigi.Identity.LebOutcome =
  var cursor: U64 = startOffset
  var acc: U64 = 0
  var shift: U64 = 0
  var stepCount: U64 = 0
  var done: Bool = false
  var outcome: Stigi.Identity.LebOutcome = Stigi.Identity.LebOutcome.LebError(startOffset, Stigi.Identity.DecodeReason.ReasonTruncated)
  while stepCount < 10 && !done do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, endOffset), lessOrEqual(stepCount, 10), lessOrEqual(shift, 63), decreases(10 - stepCount)>
    if cursor >= endOffset then
      outcome = Stigi.Identity.LebOutcome.LebError(cursor, Stigi.Identity.DecodeReason.ReasonTruncated)
      done = true
    else
      val rawByte: U8 = input(cursor)
      val payload: U64 = U64.bitwiseAnd(U64.ofU8(rawByte), 127)
      val hasMore: Bool = U64.bitwiseAnd(U64.ofU8(rawByte), 128) != 0
      if stepCount == 9 && (hasMore || payload > 1) then
        outcome = Stigi.Identity.LebOutcome.LebError(cursor, Stigi.Identity.DecodeReason.ReasonNumberTooLarge)
        done = true
      else
        if !hasMore && stepCount > 0 && payload == 0 then
          outcome = Stigi.Identity.LebOutcome.LebError(cursor, Stigi.Identity.DecodeReason.ReasonNonMinimalNumber)
          done = true
        else
          val shifted: U64 = U64.shiftLeft(payload, shift)
          acc = U64.bitwiseOr(acc, shifted)
          cursor = cursor + 1
          if !hasMore then
            outcome = Stigi.Identity.LebOutcome.LebDecoded(acc, cursor)
            done = true
          else
            val nextShift: U64 = shift + 7
            if nextShift <= 63 then
              shift = nextShift
            else
              done = true
    stepCount = stepCount + 1
  outcome
