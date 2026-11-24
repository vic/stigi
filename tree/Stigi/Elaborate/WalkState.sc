// prompt: the state of one elaboration walk: still walking, the work stack is empty, a table overflowed, or the syntax is invalid.
enum Stigi.Elaborate.WalkState:
  case Walking
  case WalkFinished
  case WalkOverflow
  case WalkInvalid
