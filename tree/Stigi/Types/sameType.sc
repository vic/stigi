// prompt: whether the interned type at one index equals a candidate normal type.
def Stigi.Types.sameType(table: Stigi.Types.TypeTable, typeAt: Stigi.Core.TypeAt, candidate: Stigi.Core.Type): Bool =
  table match
    case TypeRows(typeCapacity, types, typeEnd, _, _, _, _, _, _, _, _, _, _, _, _, _, _) =>
      if typeAt < typeEnd then
        @<below(typeAt, typeCapacity)>
        types(typeAt) match
          case Primitive(leftKind) =>
            candidate match
              case Primitive(rightKind) => Stigi.Types.primitiveHash(leftKind) == Stigi.Types.primitiveHash(rightKind)
              case Named(_, _, _) | Variable(_) => false
          case Variable(leftLevel) =>
            candidate match
              case Variable(rightLevel) => leftLevel == rightLevel
              case Primitive(_) | Named(_, _, _) => false
          case Named(leftUnit, leftArguments, leftLengths) =>
            candidate match
              case Named(rightUnit, rightArguments, rightLengths) => Stigi.Types.sameReference(leftUnit, rightUnit) && Stigi.Types.sameArguments(table, leftArguments, rightArguments) && Stigi.Types.sameLengths(table, leftLengths, rightLengths)
              case Primitive(_) | Variable(_) => false
      else false
