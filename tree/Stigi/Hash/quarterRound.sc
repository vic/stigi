// prompt: execute one BLAKE3 quarter-round mixing four state lanes in place.
def Stigi.Hash.quarterRound {Stigi.Mut} (stateLen: U64, state: Array[U32](stateLen), laneFirst: U64, laneSecond: U64, laneThird: U64, laneFourth: U64, wordFirst: U32, wordSecond: U32) <below(laneFirst, stateLen), below(laneSecond, stateLen), below(laneThird, stateLen), below(laneFourth, stateLen)>: Unit =
  val sumFirst: U32 = U32.wrappingAdd(U32.wrappingAdd(state(laneFirst), state(laneSecond)), wordFirst)
  val rotFourth: U32 = Stigi.Raw.rotateRightWord(U32.bitwiseXor(state(laneFourth), sumFirst), U32.ofU64(16))
  val sumThird: U32 = U32.wrappingAdd(state(laneThird), rotFourth)
  val rotSecond: U32 = Stigi.Raw.rotateRightWord(U32.bitwiseXor(state(laneSecond), sumThird), U32.ofU64(12))
  val outFirst: U32 = U32.wrappingAdd(U32.wrappingAdd(sumFirst, rotSecond), wordSecond)
  val outFourth: U32 = Stigi.Raw.rotateRightWord(U32.bitwiseXor(rotFourth, outFirst), U32.ofU64(8))
  val outThird: U32 = U32.wrappingAdd(sumThird, outFourth)
  val outSecond: U32 = Stigi.Raw.rotateRightWord(U32.bitwiseXor(rotSecond, outThird), U32.ofU64(7))
  state(laneFirst) := outFirst
  state(laneSecond) := outSecond
  state(laneThird) := outThird
  state(laneFourth) := outFourth
  ()
