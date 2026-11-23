// prompt: closed result returned by Stigi.Emit.emitValBind indicating whether remaining block statements were consumed.
enum Stigi.Emit.ValBindOutcome:
  case BindContinue(nextLocals: U64)
  case BindConsumed
