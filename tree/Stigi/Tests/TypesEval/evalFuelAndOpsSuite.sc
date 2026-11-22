// prompt: test compile-time evaluator arithmetic, loop fuel exhaustion, and extern rejection.
val Stigi.Tests.TypesEval.evalFuelAndOpsSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val evalValid: Bool = Array.tryFill{Stigi.Region}(6, zeroByte) match
    case FillFailed => false
    case Filled(ops) =>
      Array.tryFill{Stigi.Region}(6, zeroWord) match
        case FillFailed => false
        case Filled(destReg) =>
          Array.tryFill{Stigi.Region}(6, zeroWord) match
            case FillFailed => false
            case Filled(argLeft) =>
              Array.tryFill{Stigi.Region}(6, zeroWord) match
                case FillFailed => false
                case Filled(argRight) =>
                  ops(0) := 0
                  destReg(0) := 0
                  argLeft(0) := 40
                  ops(1) := 0
                  destReg(1) := 1
                  argLeft(1) := 2
                  ops(2) := 1
                  destReg(2) := 2
                  argLeft(2) := 0
                  argRight(2) := 1
                  ops(3) := 7
                  argLeft(3) := 2
                  val runSum: Stigi.Eval.EvalOutcome = Stigi.Eval.runProgram{Stigi.Region}(6, ops, destReg, argLeft, argRight, 4, 4, Stigi.Eval.fuelBudget)
                  val sumValid: Bool = runSum match
                    case EvalValue(word, fuel) => word == 42 && fuel == 4
                    case EvalOutOfFuel => false
                    case EvalNotCompileTime => false
                    case EvalArithmeticError => false
                  ops(0) := 0
                  destReg(0) := 0
                  argLeft(0) := 0
                  ops(1) := 5
                  argLeft(1) := 0
                  argRight(1) := 0
                  val runFuel: Stigi.Eval.EvalOutcome = Stigi.Eval.runProgram{Stigi.Region}(6, ops, destReg, argLeft, argRight, 2, 4, 20)
                  val fuelExhausted: Bool = runFuel match
                    case EvalOutOfFuel => true
                    case EvalValue(_, _) => false
                    case EvalNotCompileTime => false
                    case EvalArithmeticError => false
                  ops(0) := 6
                  val runExtern: Stigi.Eval.EvalOutcome = Stigi.Eval.runProgram{Stigi.Region}(6, ops, destReg, argLeft, argRight, 1, 4, 20)
                  val externRejected: Bool = runExtern match
                    case EvalNotCompileTime => true
                    case EvalValue(_, _) => false
                    case EvalOutOfFuel => false
                    case EvalArithmeticError => false
                  sumValid && fuelExhausted && externRejected
  Stigi.Test.expect(evalValid, true, "Evaluator computes exact arithmetic, enforces fuel budget, and rejects extern calls")
