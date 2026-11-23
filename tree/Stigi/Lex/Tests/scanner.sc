// prompt: verify full L0 tokenization of traits, pipes, multi-head cases, and digit decoders.
val Stigi.Lex.Tests.scanner {Stigi.Test}: Stigi.Test.Result =
  val decValid: Bool = Stigi.Lex.decodeDec(55) match
    case DecValue(digit) =>
      Stigi.Lex.Laws.decDigitBound(digit)
      digit == 7
    case NotDec => false
  val hexValid: Bool = Stigi.Lex.decodeHex(102) match
    case HexValue(nibble) => nibble == 15
    case NotHex => false
  val sample: Array[U8](41) = "trait Eq [A: Type]:\n  val x = a |> f(10)\n"
  val scanValid: Bool =
    val Filled(tokBuf) = Array.tryFill[Stigi.Lex.Token]{Stigi.Region}(64, Token(TokEnd, 0, 0, 0, 0)) else
      case FillFailed => false
    val LexDone(tokCount) = Stigi.Lex.scanAll(41, sample, 41, 64, tokBuf) else
      case LexOverflow => false
      case LexSyntaxError(_, _) => false
    if tokCount >= 18 then
      val Token(kindZero, _, _, _, _) = tokBuf(0)
      val Token(kindPipe, _, _, _, _) = tokBuf(13)
      kindZero == TokTrait && kindPipe == TokPipe
    else false
  Stigi.Test.expect(decValid && hexValid && scanValid, true, "L0 lexer must tokenize traits, pipes, and proved digits")
