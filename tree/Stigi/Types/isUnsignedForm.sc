// prompt: check that a normalized linear form has a non-negative U64 constant and non-negative U64 coefficients within unit node bounds.
def Stigi.Types.isUnsignedForm(poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), form: Stigi.Types.Linear.Form): Bool =
  form match
    case LinearForm(WideInt(_, constantHigh), termStart, termCount) =>
      if constantHigh != 0 then false else if termStart < 65536 && termCount < 65536 then
        val termEnd: U64 = termStart + termCount
        if termEnd <= poolCap then
          var pos: U64 = termStart
          var valid: Bool = true
          while pos < termEnd && valid do
            @<lessOrEqual(termStart, pos), lessOrEqual(pos, termEnd)>
            pool(pos) match
              case LinearTerm(binder, WideInt(_, coeffHigh)) =>
                if coeffHigh != 0 || Stigi.Limit.maxUnitNodes <= binder then valid = false else ()
            pos = pos + 1
          valid
        else false
      else false
