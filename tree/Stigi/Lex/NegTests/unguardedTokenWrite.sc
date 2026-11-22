// prompt: writing to a token array without proving capacity bound is rejected.
val Stigi.Lex.NegTests.unguardedTokenWrite {Stigi.NegTest}: Never =
  def storeToken {Stigi.Mut} (capacity: U64, tokens: Array[U64](capacity), position: U64, tokenKind: U64) <lessOrEqual(0, capacity)>: Unit =
    tokens(position) := tokenKind
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "token write index is not bounded by capacity")
