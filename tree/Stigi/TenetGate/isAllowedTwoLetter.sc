// prompt: check whether two lowercase ASCII bytes form an allowed two-letter preposition.
def Stigi.TenetGate.isAllowedTwoLetter(firstByte: U8, secondByte: U8) <lessOrEqual(0, 1)>: Bool =
  val groupOne: Bool = firstByte == 111 && secondByte == 102 || firstByte == 97 && secondByte == 116 || firstByte == 116 && secondByte == 111
  val groupTwo: Bool = firstByte == 105 && secondByte == 110 || firstByte == 105 && secondByte == 115 || firstByte == 98 && secondByte == 121
  val groupThree: Bool = firstByte == 111 && secondByte == 114 || firstByte == 111 && secondByte == 110 || firstByte == 105 && secondByte == 102 || firstByte == 97 && secondByte == 115
  groupOne || groupTwo || groupThree
