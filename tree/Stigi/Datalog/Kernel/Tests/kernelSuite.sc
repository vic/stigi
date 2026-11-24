// prompt: verify real bounds/M4, ratchet S143, and lemma-assumes S144 kernel rules on Stigi.Datalog.Engine.
val Stigi.Datalog.Kernel.Tests.kernelSuite {Stigi.Test}: Stigi.Test.Result =
  val chkQuotGood: Bool = Stigi.Datalog.Kernel.runBoundsAndQuotient{Stigi.Region}(2, false)
  val chkQuotOne: Bool = Stigi.Datalog.Kernel.runBoundsAndQuotient{Stigi.Region}(1, true)
  val chkRatLoad: Bool = Stigi.Datalog.Kernel.runRatchet{Stigi.Region}(false, 0, false)
  val chkRatEdit: Bool = Stigi.Datalog.Kernel.runRatchet{Stigi.Region}(true, 1, true)
  val chkRatUnfreeze: Bool = Stigi.Datalog.Kernel.runRatchet{Stigi.Region}(true, 2, true)
  val chkRatShared: Bool = Stigi.Datalog.Kernel.runRatchet{Stigi.Region}(true, 3, false)
  val chkLemDirect: Bool = Stigi.Datalog.Kernel.runLemmaAssumes{Stigi.Region}(0, true)
  val chkLemHelper: Bool = Stigi.Datalog.Kernel.runLemmaAssumes{Stigi.Region}(1, true)
  val chkLemOther: Bool = Stigi.Datalog.Kernel.runLemmaAssumes{Stigi.Region}(2, false)
  val partOne: Bool = chkQuotGood && chkQuotOne && chkRatLoad && chkRatEdit && chkRatUnfreeze
  val partTwo: Bool = chkRatShared && chkLemDirect && chkLemHelper && chkLemOther
  val allPass: Bool = partOne && partTwo
  Stigi.Test.expect(allPass, true, "Stigi.Datalog.Kernel must agree with tupu on M4, ratchet S143, and lemma-assumes S144")
