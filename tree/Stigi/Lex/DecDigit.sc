// prompt: parse-dont-validate decimal digit outcome carrying proved bound below ten.
enum Stigi.Lex.DecDigit:
  case DecValue(digit: U64) <below(digit, 10)>
  case NotDec
