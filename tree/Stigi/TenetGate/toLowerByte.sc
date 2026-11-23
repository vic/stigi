// prompt: fold an ASCII uppercase byte to lowercase preserving non-decreasing byte order.
def Stigi.TenetGate.toLowerByte(byte: U8): U8 <lessOrEqual(byte, result)> =
  case 'A'..'Z' => byte + ('a' - 'A')
  case _ => byte
