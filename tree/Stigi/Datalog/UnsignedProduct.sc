// prompt: exact 64 by 64 bit unsigned multiplication returning low and high 64-bit limbs using native shift and mask instructions.
enum Stigi.Datalog.UnsignedProduct:
  case WideProduct(lowWord: U64, highWord: U64)
