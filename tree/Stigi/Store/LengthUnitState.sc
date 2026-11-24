// prompt: mutable scratch, cursors, and lanes carried while walking one unit for length terms.
enum Stigi.Store.LengthUnitState:
  case LengthUnitState(scratch: Stigi.Store.LengthScratch, localCount: U64, termCount: U64, poolEnd: U64, workTop: U64, lanes: Stigi.Store.LengthLanes) <lessOrEqual(localCount, 256), lessOrEqual(termCount, 256), lessOrEqual(poolEnd, 1024), lessOrEqual(workTop, 4096)>
