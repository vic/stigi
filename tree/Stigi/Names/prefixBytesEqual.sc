// prompt: compare prefixes of length prefixLen across two byte arrays.
def Stigi.Names.prefixBytesEqual(leftLen: U64, left: Array[U8](leftLen), rightLen: U64, right: Array[U8](rightLen), prefixLen: U64) <lessOrEqual(prefixLen, leftLen), lessOrEqual(prefixLen, rightLen)>: Bool =
  var index: U64 = 0
  var matched: Bool = true
  while index < prefixLen && matched do
    @<lessOrEqual(index, prefixLen), decreases(prefixLen - index)>
    if left(index) != right(index) then
      matched = false
      index = index + 1
    else
      index = index + 1
  matched
