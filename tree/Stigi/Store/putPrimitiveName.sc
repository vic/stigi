// prompt: write the canonical L0 type name of a core primitive into a bounded buffer at a position, giving the new end.
def Stigi.Store.putPrimitiveName {Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, primitive: Stigi.Core.Primitive) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  primitive match
    case UnsignedByte => Stigi.Array.copy[U8](2, "U8", 0, 2, bufCap, buf, at)
    case UnsignedWord => Stigi.Array.copy[U8](3, "U32", 0, 3, bufCap, buf, at)
    case UnsignedLong => Stigi.Array.copy[U8](3, "U64", 0, 3, bufCap, buf, at)
    case SignedWord => Stigi.Array.copy[U8](3, "I32", 0, 3, bufCap, buf, at)
    case SignedLong => Stigi.Array.copy[U8](3, "I64", 0, 3, bufCap, buf, at)
    case Boolean => Stigi.Array.copy[U8](4, "Bool", 0, 4, bufCap, buf, at)
    case UnitValue => Stigi.Array.copy[U8](4, "Unit", 0, 4, bufCap, buf, at)
    case NeverValue => Stigi.Array.copy[U8](5, "Never", 0, 5, bufCap, buf, at)
