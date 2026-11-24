// prompt: return the exact inclusive 128-bit bounds and bit width of an L0 primitive type.
pub def Stigi.Types.integerInfoOf(primitive: Stigi.Core.Primitive): Stigi.Types.IntegerInfo =
  primitive match
    case UnsignedByte => IntSpec(WideInt(0, 0), WideInt(255, 0), WidthEight)
    case UnsignedWord => IntSpec(WideInt(0, 0), WideInt(4294967295, 0), WidthThirtyTwo)
    case UnsignedLong => IntSpec(WideInt(0, 0), WideInt(18446744073709551615, 0), WidthSixtyFour)
    case SignedWord => IntSpec(WideInt(U64.wrappingSubtract(0, 2147483648), -1), WideInt(2147483647, 0), WidthThirtyTwo)
    case SignedLong => IntSpec(WideInt(9223372036854775808, -1), WideInt(9223372036854775807, 0), WidthSixtyFour)
    case Boolean | UnitValue | NeverValue => NonInteger
