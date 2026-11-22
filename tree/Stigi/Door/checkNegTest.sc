// prompt: verify exact rejection code set equality and unique subject multiplicity for a NegTest.
pub def Stigi.Door.checkNegTest(expectedCap: U64, expectedCodes: Array[U64](expectedCap), expectedCount: U64, actualCap: U64, actualCodes: Array[U64](actualCap), actualCount: U64) <lessOrEqual(expectedCount, expectedCap), below(expectedCap, 256), lessOrEqual(actualCount, actualCap), below(actualCap, 256)>: Stigi.Door.NegTestOutcome =
  if actualCount == 0 then
    NegTestNotRejected
  else
    var outerIndex: U64 = 0
    var missingCode: Bool = false
    var duplicateSubject: Bool = false
    while outerIndex < expectedCount do
      @<lessOrEqual(outerIndex, expectedCount), decreases(expectedCount - outerIndex)>
      val targetCode: U64 = expectedCodes(outerIndex)
      val matchCount: U64 = Stigi.Door.countCodeMatches(actualCap, actualCodes, actualCount, targetCode)
      if matchCount == 0 then
        missingCode = true
      else if matchCount > 1 then
        duplicateSubject = true
      else
        ()
      outerIndex = outerIndex + 1
    if duplicateSubject then
      NegTestMultipleSubjects
    else if missingCode || actualCount != expectedCount then
      NegTestWrongCode
    else
      NegTestPassed
