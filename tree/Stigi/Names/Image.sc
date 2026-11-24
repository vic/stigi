// prompt: the names image: the name pool, the unit entries and their count, and the open-addressed table of unit names.
pub enum Stigi.Names.Image:
  case Image(pool: Array[U8](Stigi.Limit.maxIndexBytes), entries: Array[Stigi.Names.Entry](Stigi.Limit.maxUnits), count: U64, slots: Array[Stigi.Names.KeySlot](Stigi.Names.tableSlots)) <lessOrEqual(count, Stigi.Limit.maxUnits)>
