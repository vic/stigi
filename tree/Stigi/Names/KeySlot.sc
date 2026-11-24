// prompt: one slot of an open-addressed key table: vacant, or holding a key.
enum Stigi.Names.KeySlot:
  case Vacant
  case Keyed(key: U64)
