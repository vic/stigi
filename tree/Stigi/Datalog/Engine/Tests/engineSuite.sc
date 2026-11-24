// prompt: verify stratified semi-naive evaluation, exact 128-bit arithmetic, budget guard, order independence, and O2 refusal.
val Stigi.Datalog.Engine.Tests.engineSuite {Stigi.Test}: Stigi.Test.Result =
  val chkReach: Bool = Stigi.Datalog.Engine.Tests.runReachGraph{Stigi.Region}(false, 64)
  val chkReverse: Bool = Stigi.Datalog.Engine.Tests.runReachGraph{Stigi.Region}(true, 64)
  val chkBudget: Bool = Stigi.Datalog.Engine.Tests.runReachGraph{Stigi.Region}(false, 2)
  val chkNumRefuse: Bool = Stigi.Datalog.Engine.Tests.testNumericAndRefusal{Stigi.Region}(2)
  val chkArith: Bool = Stigi.Datalog.Engine.Tests.testArithAndErrors{Stigi.Region}(3)
  val allPass: Bool = chkReach && chkReverse && chkBudget && chkNumRefuse && chkArith
  Stigi.Test.expect(allPass, true, "Stigi.Datalog.Engine must pass TC+negation, 128-bit arith/errors, budget, O7 order, and O2 refusal")
