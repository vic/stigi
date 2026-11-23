// prompt: whether four lowercase bytes spell a forbidden four-letter tenet-gate token.
def Stigi.TenetGate.bannedFour(first: U8, second: U8, third: U8, fourth: U8): Bool =
  case ('t', 'r', 'a', 'p') | ('t', 'o', 'd', 'o') | ('s', 'k', 'i', 'p') | ('s', 't', 'u', 'b') => true
  case (_, _, _, _) => false
