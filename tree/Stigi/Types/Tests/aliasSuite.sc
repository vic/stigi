// prompt: verify a type without facts interns to the index of its right-hand side.
val Stigi.Types.Tests.aliasSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Types.Tests.aliasProbe{Stigi.Region}(16), true, "a type without facts must be its right-hand side")
