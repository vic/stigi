// prompt: stage one normal linear length form as one length row and its unsigned linear parts at the ends of the length and part lists.
def Stigi.Types.stageLength {Stigi.Mut} (table: Stigi.Types.TypeTable, poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), form: Stigi.Types.Linear.Form): Stigi.Types.Staged =
  table match
    case TypeRows(typeCapacity, types, typeEnd, slotCapacity, slots, listCapacity, typeLists, listMark, listEnd, lengthCapacity, lengths, lengthMark, lengthEnd, partCapacity, parts, partMark, partEnd) =>
      form match
        case LinearForm(WideInt(constant, constantHigh), termStart, termCount) =>
          if constantHigh == 0 then
            var termPosition: U64 = termStart
            var partPosition: U64 = partEnd
            var remaining: U64 = termCount
            var copy: Stigi.Types.Copy = Copying
            while remaining > 0 && copy is Copying do
              @<lessOrEqual(partEnd, partPosition), lessOrEqual(partPosition, partCapacity)>
              val nextRemaining: U64 = remaining - 1
              if termPosition < poolCap && partPosition < partCapacity then
                val nextTerm: U64 = termPosition + 1
                val nextPart: U64 = partPosition + 1
                val termCopy: Stigi.Types.Copy = pool(termPosition) match
                  case LinearTerm(binder, WideInt(coefficient, coefficientHigh)) =>
                    if coefficientHigh != 0 then CopyNegative else if binder < Stigi.Limit.maxUnitNodes then
                      parts(partPosition) := Part(coefficient, binder)
                      Copying
                    else CopyBadBinder
                termCopy match
                  case Copying =>
                    termPosition = nextTerm
                    partPosition = nextPart
                  case CopyFull | CopyNegative | CopyBadBinder => copy = termCopy
              else
                copy = CopyFull
              remaining = nextRemaining
            copy match
              case CopyFull => StageFull
              case CopyNegative => StageNotUnsigned
              case CopyBadBinder => StageBadBinder
              case Copying =>
                if lengthEnd < lengthCapacity then
                  val nextLength: U64 = lengthEnd + 1
                  lengths(lengthEnd) := Linear(constant, CoreSpan(partEnd, partPosition))
                  StagedRows(TypeRows(typeCapacity, types, typeEnd, slotCapacity, slots, listCapacity, typeLists, listMark, listEnd, lengthCapacity, lengths, lengthMark, nextLength, partCapacity, parts, partMark, partPosition), CoreSpan(lengthEnd, nextLength))
                else StageFull
          else StageNotUnsigned
