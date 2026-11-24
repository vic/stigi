// prompt: intern one enum and two variables, each equal one twice, and report whether equal types share an index and different ones do not.
def Stigi.Types.Tests.twiceProbe {region: Stigi.Region, Stigi.Mut} (slotCapacity: U64) <below(0, slotCapacity)>: Bool =
  val TableReady(table) = Stigi.Types.allocateTable{region}(8, slotCapacity, 8, 8, 8) else
    case TableUnavailable => false
  val enumType: Stigi.Core.Type = Stigi.Types.normalize(EnumType(ExternalRef(3, 0), CoreSpan(0, 0), CoreSpan(0, 0)))
  val InternedAt(firstTable, firstEnum) = Stigi.Types.intern(table, enumType) else
    case TypeTableFull => false
  val InternedAt(secondTable, secondEnum) = Stigi.Types.intern(firstTable, enumType) else
    case TypeTableFull => false
  val InternedAt(thirdTable, firstLevel) = Stigi.Types.intern(secondTable, Variable(2)) else
    case TypeTableFull => false
  val InternedAt(fourthTable, secondLevel) = Stigi.Types.intern(thirdTable, Variable(2)) else
    case TypeTableFull => false
  val InternedAt(finalTable, otherLevel) = Stigi.Types.intern(fourthTable, Variable(3)) else
    case TypeTableFull => false
  firstEnum == secondEnum && firstLevel == secondLevel && otherLevel != firstLevel && firstEnum != firstLevel && !Stigi.Types.sameType(finalTable, firstLevel, Variable(3)) && !Stigi.Types.sameType(finalTable, firstEnum, Variable(2))
