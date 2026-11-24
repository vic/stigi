// prompt: what the aliases of a unit make of one reference: nothing, or the target run of the first alias that matches with the tail of the reference that remains.
enum Stigi.Names.AliasHit:
  case Unaliased
  case AliasHit(target: Stigi.Syntax.Text, tail: Stigi.Syntax.Text)
