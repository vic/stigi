// prompt: represent a flat Datalog tuple row up to arity seven.
pub enum Stigi.Datalog.Store.Tuple:
  case StoreRow(relNum: U64, arity: U64, colZero: Stigi.Datalog.Store.Cell, colOne: Stigi.Datalog.Store.Cell, colTwo: Stigi.Datalog.Store.Cell, colThree: Stigi.Datalog.Store.Cell, colFour: Stigi.Datalog.Store.Cell, colFive: Stigi.Datalog.Store.Cell, colSix: Stigi.Datalog.Store.Cell) <lessOrEqual(arity, 7)>
