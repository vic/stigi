// prompt: verify that 42-level nested patterns and bar-patterns round-trip while 43-level fail TooDeep.
val Stigi.Syntax.Print.Tests.patternDepthSuite {Stigi.Test}: Stigi.Test.Result =
  val patternsPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestPattern, 42, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestPattern, 43, false)
  val alternativesPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestPatOr, 42, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestPatOr, 43, false)
  Stigi.Test.expect(patternsPassed && alternativesPassed, true, "42-level nested patterns and bar alternatives must round-trip and 43-level must reject TooDeep")
