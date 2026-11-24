// prompt: verify a catch-all binder is-test lowers to one true rest arm.
val Stigi.Elaborate.Tests.isBindShape {Stigi.Test}: Stigi.Test.Result =
  val srcBind: Array[U8](76) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is other\n"
  val bind: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(76, srcBind, PatLocalBind)
  Stigi.Test.expect(bind is ProbeLowered(3, 1, _, RestArm(1), _), true, "Stigi.Elaborate must lower a binder is-test to one rest arm")
