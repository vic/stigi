// prompt: verify catch-all wildcard arm on a closed enum rejects with WildcardArm.
val Stigi.NegTests.wildcardArm {Stigi.NegTest}: Never =
  def Stigi.NegTests.badWildcard(item: Stigi.Lookup): Bool =
    item match
      case Found(_) => true
      case _ => false
  Stigi.NegTest.expect(Stigi.Code.WildcardArm, "closed enum match forbids wildcard arm")
