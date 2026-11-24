// prompt: verify a tuple is-test is rejected and never dropped.
val Stigi.Elaborate.Tests.isTupleRejects {Stigi.Test}: Stigi.Test.Result =
  val srcTuple: Array[U8](77) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is (a, b)\n"
  val tuple: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(77, srcTuple, PatLocalBind)
  Stigi.Test.expect(tuple is ProbeInvalid, true, "Stigi.Elaborate must reject a tuple is-test")
