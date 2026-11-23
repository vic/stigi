// prompt: decode an ASCII byte into a proved decimal digit below ten.
def Stigi.Lex.decodeDec(byte: U8): Stigi.Lex.DecDigit =
  val word: U64 = U64.ofU8(byte)
  if word >= 48 && word <= 57 then
    val digit: U64 = word - 48
    if digit < 10 then DecValue(digit@<below(digit, 10)>) else NotDec
  else NotDec
