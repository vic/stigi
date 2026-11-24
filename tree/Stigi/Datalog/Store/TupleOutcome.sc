// prompt: represent the outcome of inserting a tuple into the tuple store.
pub enum Stigi.Datalog.Store.TupleOutcome:
  case TupleAdded(rowIndex: U64, newCount: U64)
  case TupleDuplicate(rowIndex: U64)
  case TupleTableFull
