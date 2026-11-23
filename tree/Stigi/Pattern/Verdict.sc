// prompt: closed Maranget pattern matrix verification verdict.
enum Stigi.Pattern.Verdict:
  case PatternSound
  case PatternMissing(caseIndex: U64)
  case PatternRedundant(armIndex: U64)
  case PatternWildcardEnum(armIndex: U64)
