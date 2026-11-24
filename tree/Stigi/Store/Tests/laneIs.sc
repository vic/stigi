// prompt: tell whether the rows of a lane hold exactly the expected bytes and end where they end.
def Stigi.Store.Tests.laneIs(lane: Stigi.Store.Lane, litLen: U64, lit: Array[U8](litLen)) <below(litLen, Stigi.Limit.maxSourceBytes)>: Bool =
  lane match
    case Lane(size, bytes, end) => Stigi.Lex.sliceEquals(size, bytes, 0, end, litLen, lit)
