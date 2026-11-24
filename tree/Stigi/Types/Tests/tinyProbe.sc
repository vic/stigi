// prompt: intern two different types into a table with room for one and report whether the second is the typed full outcome.
def Stigi.Types.Tests.tinyProbe {region: Stigi.Region, Stigi.Mut} (slotCapacity: U64) <below(0, slotCapacity)>: Bool =
  val TableReady(table) = Stigi.Types.allocateTable{region}(1, slotCapacity, 2, 2, 2) else
    case TableUnavailable => false
  val InternedAt(afterLong, _) = Stigi.Types.intern(table, Primitive(UnsignedLong)) else
    case TypeTableFull => false
  Stigi.Types.intern(afterLong, Primitive(Boolean)) is TypeTableFull
