// prompt: check whether a two-byte ASCII pair is in the ten-word tenet allowlist.
def Stigi.TenetGate.isAllowedPair(first: U8, second: U8): Bool =
  case (111, 102) => true
  case (97, 116) => true
  case (116, 111) => true
  case (105, 110) => true
  case (105, 115) => true
  case (98, 121) => true
  case (111, 114) => true
  case (111, 110) => true
  case (105, 102) => true
  case (97, 115) => true
  case (_, _) => false
