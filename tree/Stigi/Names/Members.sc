// prompt: the member table: its text pool and fill, the members and their count, and their open-addressed slots.
enum Stigi.Names.Members:
  case Members(pool: Array[U8](Stigi.Limit.maxIndexBytes), poolEnd: U64, members: Array[Stigi.Names.Member](Stigi.Names.maxMembers), count: U64, slots: Array[Stigi.Names.KeySlot](Stigi.Names.tableSlots)) <lessOrEqual(poolEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(count, Stigi.Names.maxMembers)>
