// prompt: outcome of verifying a NegTest against its exact expected rejection codes.
pub enum Stigi.Door.NegTestOutcome:
  case NegTestPassed
  case NegTestNotRejected
  case NegTestWrongCode
  case NegTestMultipleSubjects
