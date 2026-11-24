// prompt: verify interning an equal staged Array type again drops its staged rows so the table never fills early.
val Stigi.Types.Tests.reuseSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Types.Tests.reuseProbe{Stigi.Region}(8), true, "an equal staged type must give its rows back")
