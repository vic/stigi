// prompt: one FNV-1a step: fold a byte into a hash by xor, then multiply by the prime with wrap.
def Stigi.Names.hashStep(acc: U64, byte: U64): U64 = U64.wrappingMultiply(U64.bitwiseXor(acc, byte), Stigi.Names.hashPrime)
