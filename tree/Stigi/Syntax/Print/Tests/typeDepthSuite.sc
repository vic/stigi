// prompt: verify that 63-level nested types round-trip while 64-level nested types fail TooDeep.
val Stigi.Syntax.Print.Tests.typeDepthSuite {Stigi.Test}: Stigi.Test.Result =
  val goodDeep: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestType, 63, true)
  val rejectDeep: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestType, 64, false)
  Stigi.Test.expect(goodDeep && rejectDeep, true, "63-level type must round-trip and 64-level must reject TooDeep")
