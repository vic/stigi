// prompt: verify linear length normalizer, enum layout sizing, and recursive-type cycle detection.
val Stigi.Types.Tests.linearLayoutSuite {Stigi.Test}: Stigi.Test.Result =
  val chkLinear: Bool = Stigi.Types.Tests.testLinear{Stigi.Region}(32)
  val chkLayout: Bool = Stigi.Types.Tests.testLayout{Stigi.Region}(3)
  val chkCycles: Bool = Stigi.Types.Tests.testCycles{Stigi.Region}(3)
  val allPass: Bool = chkLinear && chkLayout && chkCycles
  Stigi.Test.expect(allPass, true, "Stigi.Types Linear, LayoutCalc, and Cycles must match src/types.c parity")
