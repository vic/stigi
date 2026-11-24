// prompt: verify the hexadecimal literal 0x10 lowers to the value sixteen.
val Stigi.Elaborate.Tests.intHexLow {Stigi.Test}: Stigi.Test.Result =
  val srcLow: Array[U8](58) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  0x10\n"
  val low: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(58, srcLow, PatLocalBind)
  Stigi.Test.expect(low is ProbeLowered(_, _, IntExpr(_, 16, _, _), _, _), true, "Stigi.Elaborate must lower 0x10 to sixteen")
