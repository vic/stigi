// prompt: verify the upper case prefix literal 0X1F lowers to the value thirty one.
val Stigi.Elaborate.Tests.intHexUpper {Stigi.Test}: Stigi.Test.Result =
  val srcUpper: Array[U8](58) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  0X1F\n"
  val upper: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(58, srcUpper, PatLocalBind)
  Stigi.Test.expect(upper is ProbeLowered(_, _, IntExpr(_, 31, _, _), _, _), true, "Stigi.Elaborate must lower 0X1F to thirty one")
