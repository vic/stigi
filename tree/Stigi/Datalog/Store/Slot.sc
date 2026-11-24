// prompt: represent an open-addressing hash slot or intrusive list link.
pub enum Stigi.Datalog.Store.Slot:
  case StoreVacant
  case StoreOccupied(rowIndex: U64)
