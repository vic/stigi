// prompt: verify an overflowing decimal literal expression is rejected and never wrapped.
val Stigi.Elaborate.Tests.intOverflowRejects {Stigi.Test}: Stigi.Test.Result =
  val srcOver: Array[U8](73) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  9223372036854775808\n"
  val over: Stigi.Elaborate.Tests.Probe = Stigi.Elaborate.Tests.lowerProbe{Stigi.Region}(73, srcOver, PatLocalBind)
  Stigi.Test.expect(over is ProbeInvalid, true, "Stigi.Elaborate must reject an integer literal that overflows")
