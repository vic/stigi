// prompt: sort and combine linear length-term binder pairs canonically by binder index, dropping zero terms.
pub def Stigi.Types.normalizeLinear {Stigi.Mut} (inCap: U64, inBinders: Array[U64](inCap), inCoeffs: Array[I64](inCap), inputCount: U64, outCap: U64, outBinders: Array[U64](outCap), outCoeffs: Array[I64](outCap)) <lessOrEqual(inputCount, inCap), below(inCap, 64), lessOrEqual(inputCount, outCap), below(outCap, 64)>: U64 <lessOrEqual(result, outCap)> =
  var outCount: U64 = 0
  var inIdx: U64 = 0
  while inIdx < inputCount do
    @<lessOrEqual(inIdx, inputCount), lessOrEqual(outCount, inIdx), decreases(inputCount - inIdx)>
    val binder: U64 = inBinders(inIdx)
    val coeff: I64 = inCoeffs(inIdx)
    if coeff != 0 then
      var foundPos: U64 = outCount
      var scan: U64 = 0
      while scan < outCount && foundPos == outCount do
        @<lessOrEqual(scan, outCount), lessOrEqual(foundPos, outCount), decreases(outCount - scan)>
        foundPos = if outBinders(scan) == binder then scan else foundPos
        scan = scan + 1
      if foundPos < outCount then
        val prevCoeff: I64 = outCoeffs(foundPos)
        val updated: I64 = if prevCoeff > -1000000 && prevCoeff < 1000000 && coeff > -1000000 && coeff < 1000000 then prevCoeff + coeff else prevCoeff
        outCoeffs(foundPos) := updated
      else
        outBinders(outCount) := binder
        outCoeffs(outCount) := coeff
        outCount = outCount + 1
    else
      ()
    inIdx = inIdx + 1
  var compactWrite: U64 = 0
  var compactRead: U64 = 0
  while compactRead < outCount do
    @<lessOrEqual(compactWrite, compactRead), lessOrEqual(compactRead, outCount), decreases(outCount - compactRead)>
    if outCoeffs(compactRead) != 0 then
      outBinders(compactWrite) := outBinders(compactRead)
      outCoeffs(compactWrite) := outCoeffs(compactRead)
      compactWrite = compactWrite + 1
    else
      ()
    compactRead = compactRead + 1
  var passIdx: U64 = 0
  while passIdx < compactWrite do
    @<lessOrEqual(passIdx, compactWrite), decreases(compactWrite - passIdx)>
    var inner: U64 = passIdx + 1
    while inner < compactWrite do
      @<lessOrEqual(inner, compactWrite), decreases(compactWrite - inner)>
      if outBinders(inner) < outBinders(passIdx) then
        val tmpBinder: U64 = outBinders(passIdx)
        val tmpCoeff: I64 = outCoeffs(passIdx)
        outBinders(passIdx) := outBinders(inner)
        outCoeffs(passIdx) := outCoeffs(inner)
        outBinders(inner) := tmpBinder
        outCoeffs(inner) := tmpCoeff
      else
        ()
      inner = inner + 1
    passIdx = passIdx + 1
  compactWrite
