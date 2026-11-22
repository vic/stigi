// prompt: compare two unit names under ASCII case folding to detect case-pair collisions.
pub def Stigi.Names.caseFoldEquals(leftLen: U64, left: Array[U8](leftLen), rightLen: U64, right: Array[U8](rightLen)) <below(leftLen, 0x100000000), below(rightLen, 0x100000000)>: Bool =
  if leftLen != rightLen then
    false
  else
    var index: U64 = 0
    var matched: Bool = true
    while index < leftLen && matched do
      @<lessOrEqual(index, leftLen), decreases(leftLen - index)>
      if Stigi.TenetGate.toLowerByte(left(index)) != Stigi.TenetGate.toLowerByte(right(index)) then
        matched = false
        index = index + 1
      else
        index = index + 1
    matched
