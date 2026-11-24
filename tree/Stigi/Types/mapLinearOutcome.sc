// prompt: convert a Linear.Outcome into the matching LengthOutcome.
def Stigi.Types.mapLinearOutcome(outcome: Stigi.Types.Linear.Outcome): Stigi.Types.LengthOutcome =
  outcome match
    case LinearVal(form, nextPool) => LengthValue(form, nextPool)
    case LinearNotLinear => LengthNotLinear
    case LinearOverflow => LengthOverflow
    case LinearDivZero => LengthDivZero
    case LinearPoolFull => LengthPoolFull
