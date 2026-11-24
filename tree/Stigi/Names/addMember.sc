// prompt: record one member of the member table, copying its text from the unit source into the member pool and hashing it into the slots.
def Stigi.Names.addMember {Stigi.Mut} (members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, owner: U64, kind: Stigi.Names.MemberKind, text: Stigi.Syntax.Text) <below(owner, Stigi.Limit.maxUnits)>: Stigi.Names.Recorded =
  members match
    case Members(pool, poolEnd, table, count, slots) =>
      text match
        case Text(first, end) =>
          val Tables(source, _, _, _, _, _) = tables
          if Stigi.Names.maxMembers <= count then MembersFull else
            val newEnd: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxUnitBytes, source, first, end, Stigi.Limit.maxIndexBytes, pool, poolEnd)
            if newEnd - poolEnd < end - first then MembersFull else
              table(count) := Member(owner, kind, poolEnd, newEnd)
              if Stigi.Names.placeKey(slots, count, Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, poolEnd, newEnd, false, Stigi.Names.hashSeed, newEnd - poolEnd + 1) % Stigi.Names.tableSlots, Stigi.Names.tableSlots) then Recorded(Members(pool, newEnd, table, count + 1, slots)) else MembersFull
