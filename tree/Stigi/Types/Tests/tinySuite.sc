// prompt: verify a full type table gives the typed full outcome.
val Stigi.Types.Tests.tinySuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Types.Tests.tinyProbe{Stigi.Region}(4), true, "a full type table must say it is full")
