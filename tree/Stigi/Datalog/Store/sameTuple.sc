// prompt: test structural equality of two Datalog tuples over their active arity.
def Stigi.Datalog.Store.sameTuple(left: Stigi.Datalog.Store.Tuple, right: Stigi.Datalog.Store.Tuple): Bool =
  left match
    case StoreRow(leftRel, leftArity, leftZero, leftOne, leftTwo, leftThree, leftFour, leftFive, leftSix) =>
      @<lessOrEqual(leftArity, 7)>
      right match
        case StoreRow(rightRel, rightArity, rightZero, rightOne, rightTwo, rightThree, rightFour, rightFive, rightSix) =>
          if leftRel != rightRel || leftArity != rightArity then
            false
          else
            val matchZero: Bool = if leftArity > 0 then Stigi.Datalog.Store.sameCell(leftZero, rightZero) else true
            val matchOne: Bool = if leftArity > 1 then Stigi.Datalog.Store.sameCell(leftOne, rightOne) else true
            val matchTwo: Bool = if leftArity > 2 then Stigi.Datalog.Store.sameCell(leftTwo, rightTwo) else true
            val matchThree: Bool = if leftArity > 3 then Stigi.Datalog.Store.sameCell(leftThree, rightThree) else true
            val matchFour: Bool = if leftArity > 4 then Stigi.Datalog.Store.sameCell(leftFour, rightFour) else true
            val matchFive: Bool = if leftArity > 5 then Stigi.Datalog.Store.sameCell(leftFive, rightFive) else true
            val matchSix: Bool = if leftArity > 6 then Stigi.Datalog.Store.sameCell(leftSix, rightSix) else true
            matchZero && matchOne && matchTwo && matchThree && matchFour && matchFive && matchSix
