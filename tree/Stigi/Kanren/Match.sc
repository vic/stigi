// prompt: what one scan of the given clauses found for a goal: none, one, two, or an effectful given.
enum Stigi.Kanren.Match:
  case Unmatched
  case OneMatch(givenIndex: U64, need: Stigi.Kanren.Need)
  case TwoMatches
  case EffectMatch(givenIndex: U64)
