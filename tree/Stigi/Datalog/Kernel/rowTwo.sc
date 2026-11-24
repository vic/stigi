// prompt: build a binary ground tuple row for kernel rule oracle tests.
def Stigi.Datalog.Kernel.rowTwo(relNum: U64, colZero: Stigi.Datalog.Store.Cell, colOne: Stigi.Datalog.Store.Cell) <below(relNum, 16)>: Stigi.Datalog.Store.Tuple =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  StoreRow(relNum, 2, colZero, colOne, padCell, padCell, padCell, padCell, padCell)
