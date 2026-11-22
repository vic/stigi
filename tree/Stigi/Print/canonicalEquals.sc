// prompt: compare two source byte arrays for exact canonical equality.
pub def Stigi.Print.canonicalEquals(leftLength: U64, left: Array[U8](leftLength), rightLength: U64, right: Array[U8](rightLength)) <lessOrEqual(0, leftLength)>: Bool =
  if leftLength != rightLength then
    false
  else
    var index: U64 = 0
    var equal: Bool = true
    while index < leftLength && equal do
      @<lessOrEqual(index, leftLength), decreases(leftLength - index)>
      if left(index) != right(index) then
        equal = false
        index = index + 1
      else
        index = index + 1
    equal
