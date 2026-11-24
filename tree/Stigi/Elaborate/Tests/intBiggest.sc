// prompt: verify the largest signed decimal literal lowers to its exact value.
val Stigi.Elaborate.Tests.intBiggest {Stigi.Test}: Stigi.Test.Result =
  val srcMax: Array[U8](73) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  9223372036854775807\n"
  val biggest: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(73, srcMax, PatLocalBind)
  Stigi.Test.expect(biggest is ProbeLowered(_, _, IntExpr(_, 9223372036854775807, _, _), _, _), true, "Stigi.Elaborate must lower the largest signed literal exactly")
