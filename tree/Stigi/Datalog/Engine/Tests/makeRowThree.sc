// prompt: construct a ternary ground tuple row for engine tests.
def Stigi.Datalog.Engine.Tests.makeRowThree(relNum: U64, colZero: Stigi.Datalog.Store.Cell, colOne: Stigi.Datalog.Store.Cell, colTwo: Stigi.Datalog.Store.Cell) <below(relNum, 16)>: Stigi.Datalog.Store.Tuple =
  val padCell: Stigi.Datalog.Store.Cell = SymCell(0)
  StoreRow(relNum, 3, colZero, colOne, colTwo, padCell, padCell, padCell, padCell)
