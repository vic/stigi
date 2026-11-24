// prompt: allocate one empty type table with the given capacities, or say an array could not be allocated.
def Stigi.Types.allocateTable {region: Stigi.Region, Stigi.Mut} (typeCapacity: U64, slotCapacity: U64, listCapacity: U64, lengthCapacity: U64, partCapacity: U64) <lessOrEqual(typeCapacity, Stigi.Limit.maxUnitNodes), below(0, slotCapacity)>: Stigi.Types.TableMade =
  val Filled(types) = Array.tryFill[Stigi.Core.Type]{region}(typeCapacity, Primitive(UnitValue)) else
    case FillFailed => TableUnavailable
  val Filled(slots) = Array.tryFill[Stigi.Types.TypeSlot]{region}(slotCapacity, EmptySlot) else
    case FillFailed => TableUnavailable
  val Filled(typeLists) = Array.tryFill[Stigi.Core.TypeAt]{region}(listCapacity, 0) else
    case FillFailed => TableUnavailable
  val Filled(lengths) = Array.tryFill[Stigi.Core.Length]{region}(lengthCapacity, Linear(0, CoreSpan(0, 0))) else
    case FillFailed => TableUnavailable
  val Filled(parts) = Array.tryFill[Stigi.Core.LinearPart]{region}(partCapacity, Part(0, 0)) else
    case FillFailed => TableUnavailable
  TableReady(TypeRows(typeCapacity, types, 0, slotCapacity, slots, listCapacity, typeLists, 0, 0, lengthCapacity, lengths, 0, 0, partCapacity, parts, 0, 0))
