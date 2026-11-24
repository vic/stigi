// prompt: verify all O2 refusal codes, arithmetic literal safety, and stratified acceptance in Stigi.Datalog.Check.
val Stigi.Datalog.Check.Tests.checkSuite {Stigi.Test}: Stigi.Test.Result =
  val chkStrat: Bool = Stigi.Datalog.Check.Tests.testStratified{Stigi.Region}(4)
  val chkUnknown: Bool = Stigi.Datalog.Check.Tests.testUnknownRel{Stigi.Region}(4)
  val chkArity: Bool = Stigi.Datalog.Check.Tests.testArityBad{Stigi.Region}(2)
  val chkHead: Bool = Stigi.Datalog.Check.Tests.testUnboundHead{Stigi.Region}(2)
  val chkNeg: Bool = Stigi.Datalog.Check.Tests.testUnboundNeg{Stigi.Region}(3)
  val chkCmp: Bool = Stigi.Datalog.Check.Tests.testUnboundCmp{Stigi.Region}(2)
  val chkSelf: Bool = Stigi.Datalog.Check.Tests.testNegSelf{Stigi.Region}(2)
  val chkCycle: Bool = Stigi.Datalog.Check.Tests.testNegCycle{Stigi.Region}(3)
  val chkArith: Bool = Stigi.Datalog.Check.Tests.testArithSafety{Stigi.Region}(2)
  val partOne: Bool = chkStrat && chkUnknown && chkArity && chkHead
  val partTwo: Bool = chkNeg && chkCmp && chkSelf && chkCycle && chkArith
  val allPass: Bool = partOne && partTwo
  Stigi.Test.expect(allPass, true, "Stigi.Datalog.Check must pass all O2 safety, arith, and stratification cases")
