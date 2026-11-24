// prompt: count one enum case of a unit as a near hit, in the namespace of the current unit, or as a far hit, visible from it.
def Stigi.Names.countCase(image: Stigi.Names.Image, members: Stigi.Names.Members, from: U64, member: U64, tally: Stigi.Names.CaseTally) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Names.CaseTally =
  val CaseTally(nearOwner, nearMember, nearCount, farOwner, farMember, farCount) = tally
  members match
    case Members(_, _, table, count, _) =>
      if member < count then
        table(member) match
          case Member(owner, kind, _, _) =>
            if kind is CaseMember then
              if Stigi.Names.sameNamespace(image, owner, from) then
                if nearCount < Stigi.Names.maxMembers then CaseTally(owner, member, nearCount + 1, farOwner, farMember, farCount) else tally
              else if Stigi.Names.visibleFrom(image, owner, from) then
                if farCount < Stigi.Names.maxMembers then CaseTally(nearOwner, nearMember, nearCount, owner, member, farCount + 1) else tally
              else tally
            else tally
      else tally
