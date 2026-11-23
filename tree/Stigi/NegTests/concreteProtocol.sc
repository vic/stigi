// prompt: verify naming a concrete handler in a capability row rejects with ConcreteProtocolInSignature.
val Stigi.NegTests.concreteProtocol {Stigi.NegTest}: Never =
  def Stigi.NegTests.badHandlerSig {file: Stigi.Posix.File} (num: U64) <lessOrEqual(0, num)>: Bool = num == 0
  Stigi.NegTest.expect(Stigi.Code.ConcreteProtocolInSignature, "signatures must name protocol Stigi.File, never handler Stigi.Posix.File")
