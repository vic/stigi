// prompt: build a unary ground tuple row for kernel rule oracle tests.
def Stigi.Datalog.Kernel.rowOne(relNum: U64, colZero: Stigi.Datalog.Store.Cell) <below(relNum, 16)>: Stigi.Datalog.Store.Tuple =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  StoreRow(relNum, 1, colZero, padCell, padCell, padCell, padCell, padCell, padCell)
