// prompt: compare five case-folded bytes starting at startOffset against five ASCII constants.
def Stigi.TenetGate.matchFiveBytes(length: U64, text: Array[U8](length), startOffset: U64, firstByte: U8, secondByte: U8, thirdByte: U8, fourthByte: U8, fifthByte: U8) <lessOrEqual(startOffset, length), below(length, 0x100000000)>: Bool =
  val posZero: U64 = startOffset
  val posOne: U64 = posZero + 1
  val posTwo: U64 = posOne + 1
  val posThree: U64 = posTwo + 1
  val fifthPos: U64 = posThree + 1
  if fifthPos < length then
    val matchHead: Bool = Stigi.TenetGate.toLowerByte(text(posZero)) == firstByte && Stigi.TenetGate.toLowerByte(text(posOne)) == secondByte
    val matchTail: Bool = Stigi.TenetGate.toLowerByte(text(posTwo)) == thirdByte && Stigi.TenetGate.toLowerByte(text(posThree)) == fourthByte && Stigi.TenetGate.toLowerByte(text(fifthPos)) == fifthByte
    matchHead && matchTail
  else
    false
