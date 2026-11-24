// prompt: law proving every Datalog tuple hash slot stays strictly below table size.
pub lemma def Stigi.Datalog.Store.Laws.tupleSlotRange(row: Stigi.Datalog.Store.Tuple, tableSize: U64) <below(0, tableSize)>: <below(0, tableSize)> =
  val slot: U64 = Stigi.Datalog.Store.hashTuple(row) % tableSize
  @<below(slot, tableSize)>
