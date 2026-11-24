// prompt: compare a fixed byte prefix of two byte arrays for exact equality.
def Stigi.Syntax.Print.Tests.bytesEqual(leftCap: U64, left: Array[U8](leftCap), rightCap: U64, right: Array[U8](rightCap), count: U64) <lessOrEqual(count, leftCap), lessOrEqual(count, rightCap)>: Bool =
  var index: U64 = 0
  while index < count && left(index) == right(index) do
    @<lessOrEqual(index, count)>
    index = index + 1
  index == count
