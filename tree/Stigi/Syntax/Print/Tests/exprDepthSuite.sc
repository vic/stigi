// prompt: verify that 140-step left spines (Binary, IsTest, Call, IndexRead, AssertStmt), 61-level ifs, and 40-level parens round-trip.
val Stigi.Syntax.Print.Tests.exprDepthSuite {Stigi.Test}: Stigi.Test.Result =
  val spinesPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestBinary, 140, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestIs, 140, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestCall, 140, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestAssert, 140, true) && Stigi.Syntax.Print.Tests.checkIndexSpine{Stigi.Region}(140)
  val conditionalsPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestIf, 61, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestIf, 62, false)
  val parensPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestParen, 40, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestParen, 41, false)
  Stigi.Test.expect(spinesPassed && conditionalsPassed && parensPassed, true, "140-step spines, 61-level if, and 40-level paren must round-trip at parser bounds")
