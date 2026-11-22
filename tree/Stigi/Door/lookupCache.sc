// prompt: look up a 256-bit S119 verdict key in the columnar open-addressing cache table.
pub def Stigi.Door.lookupCache(slotCap: U64, usedSlots: Array[U8](slotCap), keyLow: Array[U64](slotCap), keyMid: Array[U64](slotCap), keyHigh: Array[U64](slotCap), keyTop: Array[U64](slotCap), verdictPassed: Array[U8](slotCap), digest: Stigi.Hash.Digest) <below(0, slotCap), below(slotCap, 0x100000000)>: Stigi.Door.CacheOutcome =
  val DigestWords(wordZero, wordOne, wordTwo, wordThree, wordFour, wordFive, wordSix, wordSeven) = digest
  val targetLow: U64 = U64.bitwiseOr(U64.ofU32(wordZero), U64.shiftLeft(U64.ofU32(wordOne), 32))
  val targetMid: U64 = U64.bitwiseOr(U64.ofU32(wordTwo), U64.shiftLeft(U64.ofU32(wordThree), 32))
  val targetHigh: U64 = U64.bitwiseOr(U64.ofU32(wordFour), U64.shiftLeft(U64.ofU32(wordFive), 32))
  val targetTop: U64 = U64.bitwiseOr(U64.ofU32(wordSix), U64.shiftLeft(U64.ofU32(wordSeven), 32))
  var cursor: U64 = U64.mix(U64.bitwiseXor(targetLow, targetHigh)) % slotCap
  var steps: U64 = 0
  var found: Bool = false
  var resultPassed: Bool = false
  while steps < slotCap do
    @<below(cursor, slotCap), lessOrEqual(steps, slotCap), decreases(slotCap - steps)>
    if usedSlots(cursor) == 0 then
      steps = slotCap
    else
      val lowMatch: Bool = keyLow(cursor) == targetLow && keyMid(cursor) == targetMid
      val highMatch: Bool = keyHigh(cursor) == targetHigh && keyTop(cursor) == targetTop
      if lowMatch && highMatch then
        found = true
        resultPassed = verdictPassed(cursor) != 0
        steps = slotCap
      else
        val nextStep: U64 = steps + 1
        steps = nextStep
        val nextCursor: U64 = cursor + 1
        cursor = if nextCursor < slotCap then nextCursor else 0
  if found then
    CacheHit(resultPassed)
  else
    CacheMiss
