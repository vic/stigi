// prompt: closed CLI command parsed from /proc/self/cmdline arguments.
enum Stigi.Store.CliAction:
  case CliRunContracts
  case CliCheckTree(dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd)>
  case CliVerifyParse(dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd)>
  case CliExportNames(dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd)>
  case CliExportNamesTo(dirStart: U64, dirEnd: U64, outStart: U64, outEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(outStart, outEnd)>
  case CliLoadEmit(dirStart: U64, dirEnd: U64, outStart: U64, outEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(outStart, outEnd)>
  case CliInvalid
