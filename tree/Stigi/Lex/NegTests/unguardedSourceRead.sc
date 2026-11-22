// prompt: verify that reading source without a below bound in a lexer helper is rejected with IndexUnproved.
val Stigi.Lex.NegTests.unguardedSourceRead {Stigi.NegTest}: Never =
  def peekBad(length: U64, source: Array[U8](length), cursor: U64) <lessOrEqual(cursor, length)>: U8 =
    source(cursor)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "source read at cursor <= length without cursor < length must be rejected")
