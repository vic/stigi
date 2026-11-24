// prompt: four bounded byte lanes and total row count for lengthTerm, linearForm, termOffset, and sameLength exports.
enum Stigi.Store.LengthLanes:
  case LengthLanes(lengthLane: Stigi.Store.Lane, linearLane: Stigi.Store.Lane, offsetLane: Stigi.Store.Lane, sameLane: Stigi.Store.Lane, rowCount: U64)
