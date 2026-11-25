// prompt: verify logical not lowers to a negation node while arithmetic minus lowers to zero minus the operand.
val Stigi.Elaborate.Tests.unarySplit {Stigi.Test}: Stigi.Test.Result =
  val srcMinus: Array[U8](56) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  -x\n"
  val srcNot: Array[U8](58) = "// prompt: pick demo.\ndef Demo.pick(x: Bool): Bool =\n  !x\n"
  val minusSub: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbeAt{Stigi.Region}(56, srcMinus, PatLocalBind, 2)
  val minusZero: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbeAt{Stigi.Region}(56, srcMinus, PatLocalBind, 1)
  val notNode: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbeAt{Stigi.Region}(58, srcNot, PatLocalBind, 1)
  val subHolds: Bool = minusSub is ProbeLowered(3, _, ExactExpr(2, CoreSub, 1, 0, _), _, _)
  val zeroHolds: Bool = minusZero is ProbeLowered(3, _, IntExpr(1, 0, 0, _), _, _)
  val notHolds: Bool = notNode is ProbeLowered(2, _, NegateExpr(1, 0), _, _)
  Stigi.Test.expect(subHolds && zeroHolds && notHolds, true, "Stigi.Elaborate must lower !x to NegateExpr and -x to 0 - x")
