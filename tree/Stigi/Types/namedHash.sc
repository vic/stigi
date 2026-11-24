// prompt: the hash of one named type over its unit, its interned argument indexes, its length constants and its linear parts.
def Stigi.Types.namedHash(table: Stigi.Types.TypeTable, unit: Stigi.Core.Reference, argumentSpan: Stigi.Core.Span, lengthSpan: Stigi.Core.Span): U64 =
  table match
    case TypeRows(_, _, _, _, _, listCapacity, typeLists, _, _, lengthCapacity, lengths, _, _, partCapacity, parts, _, _) =>
      argumentSpan match
        case CoreSpan(argumentFirst, argumentEnd) =>
          lengthSpan match
            case CoreSpan(lengthFirst, lengthEnd) =>
              var hash: U64 = Stigi.Types.referenceHash(unit)
              var position: U64 = argumentFirst
              while position < argumentEnd && position < listCapacity do
                @<lessOrEqual(argumentFirst, position)>
                hash = U64.mix(U64.bitwiseXor(hash, typeLists(position)))
                position = position + 1
              var row: U64 = lengthFirst
              while row < lengthEnd && row < lengthCapacity do
                @<lessOrEqual(lengthFirst, row)>
                lengths(row) match
                  case Linear(constant, lengthParts) =>
                    hash = U64.mix(U64.bitwiseXor(hash, constant))
                    lengthParts match
                      case CoreSpan(partFirst, partEnd) =>
                        var partAt: U64 = partFirst
                        while partAt < partEnd && partAt < partCapacity do
                          @<lessOrEqual(partFirst, partAt)>
                          parts(partAt) match
                            case Part(coefficient, binder) =>
                              hash = U64.mix(U64.bitwiseXor(U64.mix(U64.bitwiseXor(hash, coefficient)), binder))
                          partAt = partAt + 1
                row = row + 1
              hash
