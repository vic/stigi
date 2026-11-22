// prompt: compare four case-folded bytes starting at startOffset against four ASCII constants.
def Stigi.TenetGate.matchFourBytes(length: U64, text: Array[U8](length), startOffset: U64, firstByte: U8, secondByte: U8, thirdByte: U8, fourthByte: U8) <lessOrEqual(startOffset, length), below(length, 0x100000000)>: Bool =
  val posZero: U64 = startOffset
  val posOne: U64 = posZero + 1
  val posTwo: U64 = posOne + 1
  val fourthPos: U64 = posTwo + 1
  if fourthPos < length then
    val matchFirst: Bool = Stigi.TenetGate.toLowerByte(text(posZero)) == firstByte && Stigi.TenetGate.toLowerByte(text(posOne)) == secondByte
    val matchSecond: Bool = Stigi.TenetGate.toLowerByte(text(posTwo)) == thirdByte && Stigi.TenetGate.toLowerByte(text(fourthPos)) == fourthByte
    matchFirst && matchSecond
  else
    false
