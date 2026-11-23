// prompt: single-case pattern arm descriptor for Maranget exhaustiveness and redundancy checking.
enum Stigi.Pattern.ArmInfo:
  case ArmInfo(isWildcard: Bool, caseTag: U64, hasGuard: Bool)
