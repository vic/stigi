// prompt: verify a range is-test lowers to one value arm and a false rest arm.
val Stigi.Elaborate.Tests.isRangeShape {Stigi.Test}: Stigi.Test.Result =
  val srcRange: Array[U8](75) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is 1..3\n"
  val range: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(75, srcRange, PatLocalBind)
  Stigi.Test.expect(range is ProbeLowered(4, 2, _, ValueArm(1, 3, 1), RestArm(2)), true, "Stigi.Elaborate must lower a range is-test to a value arm and a rest arm")
