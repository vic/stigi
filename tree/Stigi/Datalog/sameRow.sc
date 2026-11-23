// prompt: check whether two Datalog tuple rows are identical across all four columns.
def Stigi.Datalog.sameRow(left: Stigi.Datalog.TupleRow, right: Stigi.Datalog.TupleRow): Bool =
  val TupleRow(leftRel, leftZero, leftOne, leftTwo) = left
  val TupleRow(rightRel, rightZero, rightOne, rightTwo) = right
  leftRel == rightRel && leftZero == rightZero && leftOne == rightOne && leftTwo == rightTwo
