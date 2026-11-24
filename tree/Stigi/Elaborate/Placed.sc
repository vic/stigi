// prompt: represent the outcome of placing one core expression: the tables given back with the index of the placed expression, or a full table, or invalid children.
enum Stigi.Elaborate.Placed:
  case PlacedAt(core: Stigi.Core.Tables, at: U64)
  case PlaceFull
  case PlaceInvalid
