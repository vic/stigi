// prompt: verify Array of U8 with length n plus one interns equal in both term orders and apart from length n.
val Stigi.Types.Tests.linearSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Types.Tests.linearProbe{Stigi.Region}(16), true, "linear normal forms must decide length equality")
