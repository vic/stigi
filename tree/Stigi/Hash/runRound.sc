// prompt: execute one full BLAKE3 column and diagonal round over sixteen state words.
def Stigi.Hash.runRound {Stigi.Mut} (stateLen: U64, state: Array[U32](stateLen), msg: Array[U32](16)) <lessOrEqual(16, stateLen)>: Unit =
  Stigi.Hash.quarterRound(stateLen, state, 0, 4, 8, 12, msg(0), msg(1))
  Stigi.Hash.quarterRound(stateLen, state, 1, 5, 9, 13, msg(2), msg(3))
  Stigi.Hash.quarterRound(stateLen, state, 2, 6, 10, 14, msg(4), msg(5))
  Stigi.Hash.quarterRound(stateLen, state, 3, 7, 11, 15, msg(6), msg(7))
  Stigi.Hash.quarterRound(stateLen, state, 0, 5, 10, 15, msg(8), msg(9))
  Stigi.Hash.quarterRound(stateLen, state, 1, 6, 11, 12, msg(10), msg(11))
  Stigi.Hash.quarterRound(stateLen, state, 2, 7, 8, 13, msg(12), msg(13))
  Stigi.Hash.quarterRound(stateLen, state, 3, 4, 9, 14, msg(14), msg(15))
  ()
