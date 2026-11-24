// prompt: a pass over the rules from a rule index for one delta window of rows, with the current row total.
enum Stigi.Datalog.RulePass:
  case RulePass(ruleIdx: U64, start: U64, stop: U64, total: U64)
