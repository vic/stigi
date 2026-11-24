// prompt: intern U64 and a type without facts over U64 and report whether both share one index.
def Stigi.Types.Tests.aliasProbe {region: Stigi.Region, Stigi.Mut} (slotCapacity: U64) <below(0, slotCapacity)>: Bool =
  val TableReady(table) = Stigi.Types.allocateTable{region}(8, slotCapacity, 8, 8, 8) else
    case TableUnavailable => false
  val InternedAt(afterLong, longAt) = Stigi.Types.intern(table, Primitive(UnsignedLong)) else
    case TypeTableFull => false
  val InternedAt(_, aliasAt) = Stigi.Types.intern(afterLong, Stigi.Types.normalize(AliasType(Primitive(UnsignedLong)))) else
    case TypeTableFull => false
  aliasAt == longAt
