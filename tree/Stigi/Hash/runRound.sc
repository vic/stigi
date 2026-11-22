// prompt: run one full BLAKE3 round over columns and diagonals of the sixteen-word state.
def Stigi.Hash.runRound {Stigi.Mut} (state: Array[U32](16), message: Array[U32](16)) <lessOrEqual(0, 1)>: Unit =
  Stigi.Hash.quarterRound(state, 0, 4, 8, 12, message(0), message(1))
  Stigi.Hash.quarterRound(state, 1, 5, 9, 13, message(2), message(3))
  Stigi.Hash.quarterRound(state, 2, 6, 10, 14, message(4), message(5))
  Stigi.Hash.quarterRound(state, 3, 7, 11, 15, message(6), message(7))
  Stigi.Hash.quarterRound(state, 0, 5, 10, 15, message(8), message(9))
  Stigi.Hash.quarterRound(state, 1, 6, 11, 12, message(10), message(11))
  Stigi.Hash.quarterRound(state, 2, 7, 8, 13, message(12), message(13))
  Stigi.Hash.quarterRound(state, 3, 4, 9, 14, message(14), message(15))
