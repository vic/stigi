// prompt: hash a Datalog tuple over its relation, arity, and active columns.
def Stigi.Datalog.Store.hashTuple(row: Stigi.Datalog.Store.Tuple): U64 =
  row match
    case StoreRow(relNum, arity, colZero, colOne, colTwo, colThree, colFour, colFive, colSix) =>
      @<lessOrEqual(arity, 7)>
      val seedZero: U64 = U64.mix(U64.bitwiseXor(U64.mix(U64.wrappingAdd(relNum, 17)), arity))
      val hashZero: U64 = if arity > 0 then
        colZero match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(seedZero, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(seedZero, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else seedZero
      val hashOne: U64 = if arity > 1 then
        colOne match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(hashZero, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(hashZero, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else hashZero
      val hashTwo: U64 = if arity > 2 then
        colTwo match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(hashOne, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(hashOne, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else hashOne
      val hashThree: U64 = if arity > 3 then
        colThree match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(hashTwo, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(hashTwo, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else hashTwo
      val hashFour: U64 = if arity > 4 then
        colFour match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(hashThree, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(hashThree, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else hashThree
      val hashFive: U64 = if arity > 5 then
        colFive match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(hashFour, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(hashFour, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else hashFour
      if arity > 6 then
        colSix match
          case SymCell(symNum) => U64.mix(U64.bitwiseXor(hashFive, U64.mix(U64.wrappingAdd(symNum, 1))))
          case NumCell(WideInt(lowWord, highWord)) => U64.mix(U64.bitwiseXor(hashFive, U64.mix(U64.bitwiseXor(U64.mix(lowWord), U64.mix(Stigi.Exact.signedToBits(highWord))))))
      else hashFive
