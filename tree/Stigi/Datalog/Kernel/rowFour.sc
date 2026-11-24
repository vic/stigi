// prompt: build a four-column ground tuple row for kernel rule oracle tests.
def Stigi.Datalog.Kernel.rowFour(relNum: U64, colZero: Stigi.Datalog.Store.Cell, colOne: Stigi.Datalog.Store.Cell, colTwo: Stigi.Datalog.Store.Cell, colThree: Stigi.Datalog.Store.Cell) <below(relNum, 16)>: Stigi.Datalog.Store.Tuple =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  StoreRow(relNum, 4, colZero, colOne, colTwo, colThree, padCell, padCell, padCell)
