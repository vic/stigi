// prompt: verify the hexadecimal literal 0x1F lowers to the value thirty one.
val Stigi.Elaborate.Tests.intHexHigh {Stigi.Test}: Stigi.Test.Result =
  val srcHigh: Array[U8](58) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  0x1F\n"
  val high: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(58, srcHigh, PatLocalBind)
  Stigi.Test.expect(high is ProbeLowered(_, _, IntExpr(_, 31, _, _), _, _), true, "Stigi.Elaborate must lower 0x1F to thirty one")
