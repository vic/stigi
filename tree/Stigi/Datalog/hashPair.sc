// prompt: hash a pair of 64-bit column values using native splitmix64 finalizer U64.mix.
def Stigi.Datalog.hashPair(leftColumn: U64, rightColumn: U64) <lessOrEqual(0, leftColumn), lessOrEqual(0, rightColumn)>: U64 =
  U64.mix(U64.bitwiseXor(U64.mix(leftColumn), U64.wrappingAdd(rightColumn, 0x9e3779b97f4a7c15)))
