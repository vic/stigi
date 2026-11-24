// prompt: the first given a resolution chose, before and after the first step of the chain.
enum Stigi.Kanren.Root:
  case Unrooted
  case Rooted(givenIndex: U64)
