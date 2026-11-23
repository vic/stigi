// prompt: decode an ASCII byte into a proved decimal digit below ten.
def Stigi.Lex.decodeDec(byte: U8): Stigi.Lex.DecDigit =
  case '0'..'9' =>
    val digit: U64 = U64.ofU8(byte - '0')
    if digit < 10 then DecValue(digit@<below(digit, 10)>) else NotDec
  case _ => NotDec
