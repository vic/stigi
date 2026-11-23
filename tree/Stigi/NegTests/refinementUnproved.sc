// prompt: verify violating a nominal refinement type rejects with RefinementUnproved.
val Stigi.NegTests.refinementUnproved {Stigi.NegTest}: Never =
  def Stigi.NegTests.needCap(cap: Stigi.UnitCap): U64 = cap
  def Stigi.NegTests.badRefinement(raw: U64) <lessOrEqual(0, raw)>: U64 = Stigi.NegTests.needCap(raw)
  Stigi.NegTest.expect(Stigi.Code.RefinementUnproved, "raw is not proved below Stigi.Limit.maxUnits")
