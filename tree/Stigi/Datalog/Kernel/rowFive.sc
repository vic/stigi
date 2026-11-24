// prompt: build a five-column ground tuple row for kernel rule oracle tests.
def Stigi.Datalog.Kernel.rowFive(relNum: U64, colZero: Stigi.Datalog.Store.Cell, colOne: Stigi.Datalog.Store.Cell, colTwo: Stigi.Datalog.Store.Cell, colThree: Stigi.Datalog.Store.Cell, colFour: Stigi.Datalog.Store.Cell) <below(relNum, 16)>: Stigi.Datalog.Store.Tuple =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  StoreRow(relNum, 5, colZero, colOne, colTwo, colThree, colFour, padCell, padCell)
