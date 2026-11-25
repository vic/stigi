// prompt: verify unsigned literals up to the largest 64-bit word lower exactly and a larger literal lowers to the overflow marker instead of wrapping or rejecting.
val Stigi.Elaborate.Tests.intOverflowRejects {Stigi.Test}: Stigi.Test.Result =
  val srcSign: Array[U8](73) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  9223372036854775808\n"
  val srcMax: Array[U8](74) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  18446744073709551615\n"
  val srcHex: Array[U8](72) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  0x9e3779b97f4a7c15\n"
  val srcOver: Array[U8](74) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  18446744073709551616\n"
  val sign: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(73, srcSign, PatLocalBind)
  val wordMax: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(74, srcMax, PatLocalBind)
  val hexWord: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(72, srcHex, PatLocalBind)
  val over: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(74, srcOver, PatLocalBind)
  val signHolds: Bool = sign is ProbeLowered(_, _, IntExpr(_, 9223372036854775808, 0, _), _, _)
  val maxHolds: Bool = wordMax is ProbeLowered(_, _, IntExpr(_, 18446744073709551615, 0, _), _, _)
  val hexHolds: Bool = hexWord is ProbeLowered(_, _, IntExpr(_, 11400714819323198485, 0, _), _, _)
  val overHolds: Bool = over is ProbeLowered(_, _, IntExpr(_, 0, 1, _), _, _)
  Stigi.Test.expect(signHolds && maxHolds && hexHolds && overHolds, true, "Stigi.Elaborate must lower literals up to the largest unsigned word exactly and mark a larger literal as overflowed")
