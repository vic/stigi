// prompt: official BLAKE3 initialization vector word by lane index.
def Stigi.Hash.initWord(lane: U64) <below(lane, 8)>: U32 =
  case 0 => U32.ofU64(0x6a09e667)
  case 1 => U32.ofU64(0xbb67ae85)
  case 2 => U32.ofU64(0x3c6ef372)
  case 3 => U32.ofU64(0xa54ff53a)
  case 4 => U32.ofU64(0x510e527f)
  case 5 => U32.ofU64(0x9b05688c)
  case 6 => U32.ofU64(0x1f83d9ab)
  case _ => U32.ofU64(0x5be0cd19)
