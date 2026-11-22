// prompt: find the closing quote offset for a JSON string starting at contentStart.
def Stigi.Server.findClosingQuote(length: U64, payload: Array[U8](length), contentStart: U64) <lessOrEqual(contentStart, length), below(length, 0x100000000)>: Stigi.Server.EnvelopeOutcome =
  var endScan: U64 = contentStart
  var closed: Bool = false
  var closePos: U64 = contentStart
  while endScan < length && !closed do
    @<lessOrEqual(contentStart, endScan), lessOrEqual(endScan, length), lessOrEqual(contentStart, closePos), lessOrEqual(closePos, length), decreases(length - endScan)>
    if payload(endScan) == 34 then
      closePos = endScan
      closed = true
      endScan = length
    else
      endScan = endScan + 1
  if closed then
    EnvelopeSpan(contentStart, closePos)
  else
    EnvelopeInvalid
