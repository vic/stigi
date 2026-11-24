// prompt: intern two types with facts over U64 and U64 itself and report whether all three indexes differ.
def Stigi.Types.Tests.nominalProbe {region: Stigi.Region, Stigi.Mut} (slotCapacity: U64) <below(0, slotCapacity)>: Bool =
  val TableReady(table) = Stigi.Types.allocateTable{region}(8, slotCapacity, 8, 8, 8) else
    case TableUnavailable => false
  val longType: Stigi.Core.Type = Primitive(UnsignedLong)
  val InternedAt(afterNode, nodeAt) = Stigi.Types.intern(table, Stigi.Types.normalize(FactType(ExternalRef(1, 0), CoreSpan(0, 0), CoreSpan(0, 0), longType))) else
    case TypeTableFull => false
  val InternedAt(afterToken, tokenAt) = Stigi.Types.intern(afterNode, Stigi.Types.normalize(FactType(ExternalRef(2, 0), CoreSpan(0, 0), CoreSpan(0, 0), longType))) else
    case TypeTableFull => false
  val InternedAt(finalTable, longAt) = Stigi.Types.intern(afterToken, longType) else
    case TypeTableFull => false
  nodeAt != tokenAt && nodeAt != longAt && tokenAt != longAt && !Stigi.Types.sameType(finalTable, nodeAt, Named(ExternalRef(2, 0), CoreSpan(0, 0), CoreSpan(0, 0))) && Stigi.Types.sameType(finalTable, nodeAt, Named(ExternalRef(1, 0), CoreSpan(0, 0), CoreSpan(0, 0)))
