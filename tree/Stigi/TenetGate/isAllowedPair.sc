// prompt: check whether a two-byte ASCII pair is in the ten-word tenet allowlist.
def Stigi.TenetGate.isAllowedPair(first: U8, second: U8): Bool =
  case ('o', 'f') | ('a', 't') | ('t', 'o') | ('i', 'n') | ('i', 's') | ('b', 'y') | ('o', 'r') | ('o', 'n') | ('i', 'f') | ('a', 's') => true
  case (_, _) => false
