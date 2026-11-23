// prompt: closed compiler execution outcome mapped to process status only in main.
enum Stigi.RunOutcome:
  case RunSuccess
  case RunCliError
  case RunFileError
  case RunOutOfMemory
  case RunDoorRejected
  case RunEmitError
  case RunContractError
