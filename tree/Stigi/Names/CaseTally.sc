// prompt: the outcome of the search of unqualified enum cases: no hit yet, one hit, or more than one, in the same namespace and among visible units.
enum Stigi.Names.CaseTally:
  case CaseTally(nearOwner: U64, nearMember: U64, nearCount: U64, farOwner: U64, farMember: U64, farCount: U64)
