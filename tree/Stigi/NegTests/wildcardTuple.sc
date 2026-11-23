// prompt: verify a catch-all tuple row over closed enum columns rejects with WildcardArm.
val Stigi.NegTests.wildcardTuple {Stigi.NegTest}: Never =
  def Stigi.NegTests.badWildcardTuple(item: Stigi.Lookup, size: Stigi.File.SizeResult): Bool =
    (item, size) match
      case (Found(_), SizeBytes(_)) => true
      case (_, _) => false
  Stigi.NegTest.expect(Stigi.Code.WildcardArm, "a tuple of closed enums forbids an all-wildcard row")
