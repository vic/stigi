// prompt: advance past spaces and colons in a JSON payload and return the next offset.
def Stigi.Server.advancePastColon(length: U64, payload: Array[U8](length), startOffset: U64) <lessOrEqual(startOffset, length), below(length, 0x100000000)>: U64 <lessOrEqual(startOffset, result), lessOrEqual(result, length)> =
  var scan: U64 = startOffset
  var stopped: Bool = false
  var resultPos: U64 = length
  while scan < length && !stopped do
    @<lessOrEqual(startOffset, scan), lessOrEqual(scan, length), lessOrEqual(startOffset, resultPos), lessOrEqual(resultPos, length), decreases(length - scan)>
    val currentByte: U8 = payload(scan)
    if currentByte == 32 || currentByte == 58 then
      scan = scan + 1
    else
      resultPos = scan
      stopped = true
      scan = length
  if stopped then resultPos else scan
