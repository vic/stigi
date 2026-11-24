// prompt: read a proved column slot from a stored tuple row.
def Stigi.Datalog.Engine.rowCol(row: Stigi.Datalog.Store.Tuple, slot: U64) <below(slot, 7)>: Stigi.Datalog.Store.Cell =
  row match
    case StoreRow(_, _, colZero, colOne, colTwo, colThree, colFour, colFive, colSix) =>
      if slot == 0 then
        colZero
      else if slot == 1 then
        colOne
      else if slot == 2 then
        colTwo
      else if slot == 3 then
        colThree
      else if slot == 4 then
        colFour
      else if slot == 5 then
        colFive
      else
        colSix
