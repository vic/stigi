// prompt: the state of one argument copy: still copying, the child list is full, or an argument is outside the map.
enum Stigi.Elaborate.ArgumentCopy:
  case ArgumentsCopying
  case ArgumentsFull
  case ArgumentsBad
