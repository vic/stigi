// prompt: execute one fuel-bounded register-machine IR program deterministically with zero unchecked ops.
pub def Stigi.Eval.runProgram {region: Stigi.Region, Stigi.Mut} (instCap: U64, ops: Array[U8](instCap), destReg: Array[U64](instCap), argLeft: Array[U64](instCap), argRight: Array[U64](instCap), instCount: U64, regCount: U64, fuelLimit: U64) <below(0, instCount), lessOrEqual(instCount, instCap), below(instCap, 4096), below(0, regCount), below(regCount, 256), lessOrEqual(fuelLimit, Stigi.Eval.fuelBudget)>: Stigi.Eval.EvalOutcome =
  val zeroWord: U64 = 0
  Array.tryFill{region}(regCount, zeroWord) match
    case FillFailed => Stigi.Eval.EvalOutcome.EvalArithmeticError
    case Filled(regs) =>
      var progCounter: U64 = 0
      var fuelUsed: U64 = 0
      var halted: Bool = false
      var outcome: Stigi.Eval.EvalOutcome = Stigi.Eval.EvalOutcome.EvalOutOfFuel
      while progCounter < instCount && fuelUsed < fuelLimit && !halted do
        @<lessOrEqual(progCounter, instCount), lessOrEqual(fuelUsed, fuelLimit), decreases(fuelLimit - fuelUsed)>
        val code: U8 = ops(progCounter)
        val dst: U64 = destReg(progCounter)
        val leftVal: U64 = argLeft(progCounter)
        val rightVal: U64 = argRight(progCounter)
        fuelUsed = fuelUsed + 1
        if code == 0 then
          if dst < regCount then
            regs(dst) := leftVal
          else
            ()
          progCounter = progCounter + 1
        else
          if code == 1 then
            if dst < regCount && leftVal < regCount && rightVal < regCount then
              regs(dst) := U64.wrappingAdd(regs(leftVal), regs(rightVal))
            else
              ()
            progCounter = progCounter + 1
          else
            if code == 2 then
              if dst < regCount && leftVal < regCount && rightVal < regCount then
                regs(dst) := U64.wrappingSubtract(regs(leftVal), regs(rightVal))
              else
                ()
              progCounter = progCounter + 1
            else
              if code == 3 then
                if dst < regCount && leftVal < regCount && rightVal < regCount then
                  regs(dst) := U64.wrappingMultiply(regs(leftVal), regs(rightVal))
                else
                  ()
                progCounter = progCounter + 1
              else
                if code == 4 then
                  if dst < regCount && leftVal < regCount && rightVal < regCount then
                    val denom: U64 = regs(rightVal)
                    if denom > 0 then
                      regs(dst) := regs(leftVal) / denom
                      progCounter = progCounter + 1
                    else
                      outcome = Stigi.Eval.EvalOutcome.EvalArithmeticError
                      halted = true
                  else
                    progCounter = progCounter + 1
                else
                  if code == 5 then
                    if leftVal < regCount && regs(leftVal) == 0 && rightVal < instCount then
                      progCounter = rightVal
                    else
                      progCounter = progCounter + 1
                  else
                    if code == 6 then
                      outcome = Stigi.Eval.EvalOutcome.EvalNotCompileTime
                      halted = true
                    else
                      val retWord: U64 = if leftVal < regCount then regs(leftVal) else 0
                      outcome = Stigi.Eval.EvalOutcome.EvalValue(retWord, fuelUsed)
                      halted = true
      outcome
