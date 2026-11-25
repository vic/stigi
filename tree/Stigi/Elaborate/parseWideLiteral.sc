// prompt: parse an unsigned decimal or hexadecimal integer literal from a source byte span into a 64-bit word, reporting a value past the largest unsigned word as an overflow.
def Stigi.Elaborate.parseWideLiteral(srcCap: U64, source: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(first, endPos), lessOrEqual(endPos, srcCap)>: Stigi.Elaborate.WideInt =
  var pos: U64 = first
  var radix: U64 = 10
  var limit: U64 = 1844674407370955161
  var lastMax: U64 = 5
  var badFlag: Bool = false
  var overFlag: Bool = false
  var seenDigit: Bool = false
  var acc: U64 = 0
  if pos < endPos then
    if U64.ofU8(source(pos)) == 48 then
      val prefixAt: U64 = pos + 1
      if prefixAt < endPos then
        if U64.ofU8(source(prefixAt)) == 120 || U64.ofU8(source(prefixAt)) == 88 then
          radix = 16
          limit = 1152921504606846975
          lastMax = 15
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
    @<lessOrEqual(pos, endPos)>
    val nextPos: U64 = pos + 1
    val byteVal: U64 = U64.ofU8(source(pos))
    var nibble: U64 = 16
    if byteVal >= 48 && byteVal <= 57 then
      nibble = byteVal - 48
    else if byteVal >= 97 && byteVal <= 102 then
      nibble = byteVal - 87
    else if byteVal >= 65 && byteVal <= 70 then
      nibble = byteVal - 55
    else
      ()
    if nibble < radix then
      seenDigit = true
      if acc < limit || acc == limit && nibble <= lastMax then
        acc = U64.wrappingAdd(U64.wrappingMultiply(acc, radix), nibble)
      else
        overFlag = true
    else
      badFlag = true
    pos = nextPos
  if badFlag || !seenDigit then WideMalformed else if overFlag then WideOverflow else WideFits(acc)
