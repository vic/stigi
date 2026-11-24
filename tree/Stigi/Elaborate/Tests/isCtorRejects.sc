// prompt: verify an is-test with constructor parts is rejected and never dropped.
val Stigi.Elaborate.Tests.isCtorRejects {Stigi.Test}: Stigi.Test.Result =
  val srcCtor: Array[U8](78) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is Some(y)\n"
  val ctor: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(78, srcCtor, PatLocalBind)
  Stigi.Test.expect(ctor is ProbeInvalid, true, "Stigi.Elaborate must reject an is-test with constructor parts")
