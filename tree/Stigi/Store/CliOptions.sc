// prompt: parsed command-line spans extracted from Linux process cmdline bytes.
pub enum Stigi.Store.CliOptions:
  case CliArgs(loadStart: U64, loadEnd: U64, emitStart: U64, emitEnd: U64, checkOnly: Bool, showTests: Bool, showLaws: Bool)
