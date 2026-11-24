// prompt: one step of a member lookup: a member whose text matches and the slot to continue from, or the end of the members.
enum Stigi.Names.MemberStep:
  case MemberHit(member: U64, nextSlot: U64) <below(nextSlot, Stigi.Names.tableSlots)>
  case MembersEnded
