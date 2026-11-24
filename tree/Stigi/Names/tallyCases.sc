// prompt: walk the chain of members that carry a text and tally their enum cases by namespace.
def Stigi.Names.tallyCases(scope: Stigi.Names.NameScope, tables: Stigi.Syntax.Tables, from: U64, text: Stigi.Syntax.Text, step: Stigi.Names.MemberStep, tally: Stigi.Names.CaseTally, remaining: U64) <below(from, Stigi.Limit.maxUnits), decreases(remaining)>: Stigi.Names.CaseTally =
  if remaining < 1 then tally else
    step match
      case MembersEnded => tally
      case MemberHit(member, nextSlot) =>
        val Text(first, end) = text
        scope match
          case NameScope(image, members) => Stigi.Names.tallyCases(scope, tables, from, text, Stigi.Names.nextMember(members, tables, first, end, nextSlot, Stigi.Names.tableSlots), Stigi.Names.countCase(image, members, from, member, tally), remaining - 1)
