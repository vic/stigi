// prompt: check whether an exact outcome is overflow.
def Stigi.Exact.Tests.isExactOverflow(outcome: Stigi.Exact.Outcome): Bool =
  outcome match
    case ExactOverflow => true
    case ExactValue(_) => false
    case ExactDivideByZero => false
