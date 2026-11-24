// prompt: stage one interned type index at the end of the argument list.
def Stigi.Types.stageArgument {Stigi.Mut} (table: Stigi.Types.TypeTable, typeAt: Stigi.Core.TypeAt): Stigi.Types.Staged =
  table match
    case TypeRows(typeCapacity, types, typeEnd, slotCapacity, slots, listCapacity, typeLists, listMark, listEnd, lengthCapacity, lengths, lengthMark, lengthEnd, partCapacity, parts, partMark, partEnd) =>
      if listEnd < listCapacity then
        @<below(listEnd, listCapacity)>
        val nextEnd: U64 = listEnd + 1
        typeLists(listEnd) := typeAt
        StagedRows(TypeRows(typeCapacity, types, typeEnd, slotCapacity, slots, listCapacity, typeLists, listMark, nextEnd, lengthCapacity, lengths, lengthMark, lengthEnd, partCapacity, parts, partMark, partEnd), CoreSpan(listEnd, nextEnd))
      else StageFull
