// prompt: compare two byte arrays lexicographically returning negative one, zero, or positive one.
pub def Stigi.Identity.compareSlices(leftLen: U64, left: Array[U8](leftLen), rightLen: U64, right: Array[U8](rightLen)) <below(leftLen, 0x100000000), below(rightLen, 0x100000000)>: I32 =
  val commonLen: U64 = if leftLen < rightLen then leftLen else rightLen
  var index: U64 = 0
  var diff: I32 = 0
  while index < commonLen && diff == 0 do
    @<lessOrEqual(index, commonLen), decreases(commonLen - index)>
    val leftByte: U8 = left(index)
    val rightByte: U8 = right(index)
    if leftByte < rightByte then
      diff = -1
      index = index + 1
    else
      if leftByte > rightByte then
        diff = 1
        index = index + 1
      else
        index = index + 1
  if diff != 0 then
    diff
  else
    if leftLen < rightLen then
      -1
    else
      if leftLen > rightLen then 1 else 0
