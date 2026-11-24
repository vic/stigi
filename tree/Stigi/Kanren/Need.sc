// prompt: what a given clause requires before it holds: nothing, or one more trait goal.
enum Stigi.Kanren.Need:
  case Ground
  case Requires(traitTag: U64, argTag: U64)
