// prompt: hash a four-word Datalog tuple into a sixty-four-bit lookup key using splitmix64.
def Stigi.Datalog.tupleKey(row: Stigi.Datalog.TupleRow, tableSize: U64) <below(0, tableSize)>: U64 <below(result, tableSize)> =
  val TupleRow(relTag, colZero, colOne, colTwo) = row
  val mixZero: U64 = U64.bitwiseXor(U64.mix(relTag), U64.mix(U64.wrappingAdd(colZero, 1)))
  val mixOne: U64 = U64.bitwiseXor(U64.mix(U64.wrappingAdd(colOne, 2)), U64.mix(U64.wrappingAdd(colTwo, 3)))
  Stigi.Raw.mixSlot(U64.bitwiseXor(mixZero, mixOne), tableSize)
