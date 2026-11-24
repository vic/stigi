// prompt: verify interning an equal normal type again gives the same index and a different one does not.
val Stigi.Types.Tests.twiceSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Types.Tests.twiceProbe{Stigi.Region}(16), true, "equal normal types must share one index")
