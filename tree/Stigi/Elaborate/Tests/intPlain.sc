// prompt: verify the decimal literal 255 lowers to its value.
val Stigi.Elaborate.Tests.intPlain {Stigi.Test}: Stigi.Test.Result =
  val srcPlain: Array[U8](57) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  255\n"
  val plain: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(57, srcPlain, PatLocalBind)
  Stigi.Test.expect(plain is ProbeLowered(_, _, IntExpr(_, 255, _, _), _, _), true, "Stigi.Elaborate must lower 255 to itself")
