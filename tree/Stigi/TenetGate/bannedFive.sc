// prompt: whether five lowercase bytes spell a forbidden five-letter tenet-gate token.
def Stigi.TenetGate.bannedFive(first: U8, second: U8, third: U8, fourth: U8, fifth: U8): Bool =
  case ('a', 'b', 'o', 'r', 't') | ('p', 'a', 'n', 'i', 'c') | ('f', 'i', 'x', 'm', 'e') | ('l', 'a', 't', 'e', 'r') => true
  case (_, _, _, _, _) => false
