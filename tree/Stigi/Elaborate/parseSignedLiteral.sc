// prompt: parse a signed decimal or hexadecimal integer literal from a source byte span into a signed 64-bit integer.
def Stigi.Elaborate.parseSignedLiteral(srcCap: U64, source: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(first, endPos), lessOrEqual(endPos, srcCap)>: Stigi.Elaborate.ParsedInt =
  var pos: U64 = first
  var negative: Bool = false
  var radix: U64 = 10
  var limit: U64 = 922337203685477580
  var badFlag: Bool = false
  var seenDigit: Bool = false
  var acc: U64 = 0
  if pos < endPos then
    if U64.ofU8(source(pos)) == 45 then
      negative = true
      pos = pos + 1
    else
      ()
  else
    ()
  if pos < endPos then
    if U64.ofU8(source(pos)) == 48 then
      val prefixAt: U64 = pos + 1
      if prefixAt < endPos then
        if U64.ofU8(source(prefixAt)) == 120 || U64.ofU8(source(prefixAt)) == 88 then
          radix = 16
          limit = 1152921504606846975
          pos = prefixAt + 1
        else
          ()
      else
        ()
    else
      ()
  else
    ()
  while pos < endPos do
    @<lessOrEqual(first, pos), lessOrEqual(pos, endPos)>
    val nextPos: U64 = pos + 1
    val byteVal: U64 = U64.ofU8(source(pos))
    var nibble: U64 = 0
    if byteVal >= 48 && byteVal <= 57 then
      nibble = byteVal - 48
    else if byteVal >= 97 && byteVal <= 102 then
      nibble = byteVal - 87
    else if byteVal >= 65 && byteVal <= 70 then
      nibble = byteVal - 55
    else
      badFlag = true
    if nibble < radix && acc <= limit then
      acc = U64.wrappingAdd(U64.wrappingMultiply(acc, radix), nibble)
      seenDigit = true
    else
      badFlag = true
    pos = nextPos
  if badFlag || !seenDigit then BadIntLiteral else if negative then
    if acc > 9223372036854775808 then BadIntLiteral else ParsedInt(I64.ofU64(U64.wrappingSubtract(0, acc)))
  else
    if acc >= 9223372036854775808 then BadIntLiteral else ParsedInt(I64.ofU64(acc))
