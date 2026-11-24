// prompt: normalize a Checked-IR length expression root into an unsigned canonical linear form, rejecting non-linear or negative forms.
pub def Stigi.Types.lengthOf {Stigi.Mut} (core: Stigi.Core.Tables, context: Stigi.Types.LengthContext, exprAt: U64) <below(exprAt, Stigi.Limit.maxUnitNodes)>: Stigi.Types.LengthOutcome =
  context match
    case LengthSpace(poolCapacity, pool, _, _, _, _, _, nodeStack, stepStack, formStack) =>
      Stigi.Types.normalizeCoreLength(core, context, exprAt, nodeStack, stepStack, formStack) match
        case LengthValue(form, nextPool) =>
          if nextPool <= poolCapacity && Stigi.Types.isUnsignedForm(poolCapacity, pool, form) then LengthValue(form, nextPool) else LengthNotUnsigned
        case LengthNotLinear => LengthNotLinear
        case LengthOverflow => LengthOverflow
        case LengthDivZero => LengthDivZero
        case LengthPoolFull => LengthPoolFull
        case LengthNotUnsigned => LengthNotUnsigned
