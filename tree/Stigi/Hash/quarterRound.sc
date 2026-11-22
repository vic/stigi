// prompt: execute one BLAKE3 quarter-round mixing four state words and two message words.
def Stigi.Hash.quarterRound {Stigi.Mut} (state: Array[U32](16), indexFirst: U64, indexSecond: U64, indexThird: U64, indexFourth: U64, wordLeft: U32, wordRight: U32) <below(indexFirst, 16), below(indexSecond, 16), below(indexThird, 16), below(indexFourth, 16)>: Unit =
  val firstOne: U32 = U32.wrappingAdd(U32.wrappingAdd(state(indexFirst), state(indexSecond)), wordLeft)
  val fourthOne: U32 = U32.rotateRight(U32.bitwiseXor(state(indexFourth), firstOne), 16)
  val thirdOne: U32 = U32.wrappingAdd(state(indexThird), fourthOne)
  val secondOne: U32 = U32.rotateRight(U32.bitwiseXor(state(indexSecond), thirdOne), 12)
  val firstTwo: U32 = U32.wrappingAdd(U32.wrappingAdd(firstOne, secondOne), wordRight)
  val fourthTwo: U32 = U32.rotateRight(U32.bitwiseXor(fourthOne, firstTwo), 8)
  val thirdTwo: U32 = U32.wrappingAdd(thirdOne, fourthTwo)
  val secondTwo: U32 = U32.rotateRight(U32.bitwiseXor(secondOne, thirdTwo), 7)
  state(indexFirst) := firstTwo
  state(indexSecond) := secondTwo
  state(indexThird) := thirdTwo
  state(indexFourth) := fourthTwo
