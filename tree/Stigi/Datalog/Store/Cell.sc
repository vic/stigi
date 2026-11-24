// prompt: represent one column cell of a Datalog tuple as symbol or exact integer.
pub enum Stigi.Datalog.Store.Cell:
  case SymCell(symNum: U64)
  case NumCell(value: Stigi.Exact.IntWide)
