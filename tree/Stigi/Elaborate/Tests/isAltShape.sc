// prompt: verify an or-pattern is-test lowers to two case arms and a false rest arm.
val Stigi.Elaborate.Tests.isAltShape {Stigi.Test}: Stigi.Test.Result =
  val srcAlt: Array[U8](76) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is A | B\n"
  val alt: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(76, srcAlt, PatCaseOrd(2))
  Stigi.Test.expect(alt is ProbeLowered(4, 3, _, CaseArm(2, _, 1), CaseArm(2, _, 1)), true, "Stigi.Elaborate must lower an or-pattern is-test to two case arms and a rest arm")
