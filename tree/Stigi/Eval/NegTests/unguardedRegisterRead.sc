// prompt: reading an evaluator register without proving regCount bound is rejected.
val Stigi.Eval.NegTests.unguardedRegisterRead {Stigi.NegTest}: Never =
  def readReg(regCount: U64, regs: Array[U64](regCount), regIdx: U64) <lessOrEqual(0, regCount)>: U64 =
    regs(regIdx)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "register index is not bounded below regCount")
