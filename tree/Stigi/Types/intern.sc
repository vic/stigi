// prompt: intern one normal type: on an equal row give back its index and drop the staged rows by resetting each fill end to its mark, else append the type, index it in the hash table and commit the staged rows.
def Stigi.Types.intern {Stigi.Mut} (table: Stigi.Types.TypeTable, candidate: Stigi.Core.Type): Stigi.Types.Interned =
  val hash: U64 = candidate match
    case Primitive(kind) => Stigi.Types.primitiveHash(kind)
    case Variable(level) => U64.mix(U64.bitwiseXor(Stigi.Types.hashSeed, level))
    case Named(unit, arguments, lengths) => Stigi.Types.namedHash(table, unit, arguments, lengths)
  table match
    case TypeRows(typeCapacity, types, typeEnd, slotCapacity, slots, listCapacity, typeLists, listMark, listEnd, lengthCapacity, lengths, lengthMark, lengthEnd, partCapacity, parts, partMark, partEnd) =>
      var slotAt: U64 = hash % slotCapacity
      var probe: U64 = 0
      var search: Stigi.Types.Probe = Probing
      while probe < slotCapacity && search is Probing do
        @<below(slotAt, slotCapacity)>
        val nextProbe: U64 = probe + 1
        slots(slotAt) match
          case EmptySlot => search = FoundVacant
          case TypeHeld(heldAt) =>
            if Stigi.Types.sameType(table, heldAt, candidate) then
              search = FoundEqual(heldAt)
            else
              val stepped: U64 = slotAt + 1
              slotAt = stepped % slotCapacity
        probe = nextProbe
      search match
        case FoundEqual(heldAt) => InternedAt(TypeRows(typeCapacity, types, typeEnd, slotCapacity, slots, listCapacity, typeLists, listMark, listMark, lengthCapacity, lengths, lengthMark, lengthMark, partCapacity, parts, partMark, partMark), heldAt)
        case FoundVacant =>
          if typeEnd < typeCapacity then
            val nextEnd: U64 = typeEnd + 1
            types(typeEnd) := candidate
            slots(slotAt) := TypeHeld(typeEnd)
            InternedAt(TypeRows(typeCapacity, types, nextEnd, slotCapacity, slots, listCapacity, typeLists, listEnd, listEnd, lengthCapacity, lengths, lengthEnd, lengthEnd, partCapacity, parts, partEnd, partEnd), typeEnd)
          else TypeTableFull
        case Probing => TypeTableFull
