// prompt: map a closed compiler outcome to its non-negative process status at main.
pub def Stigi.outcomeStatus(outcome: Stigi.RunOutcome): I32 <lessOrEqual(0, result)> =
  case RunSuccess => 0
  case RunCliError => 2
  case RunFileError => 3
  case RunOutOfMemory => 4
  case RunDoorRejected => 5
  case RunEmitError => 6
  case RunContractError => 7
