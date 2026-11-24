// prompt: verify two types with facts over U64 intern to two indexes, neither equal to U64.
val Stigi.Types.Tests.nominalSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Types.Tests.nominalProbe{Stigi.Region}(16), true, "two types with facts over U64 must stay nominal")
