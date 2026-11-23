// prompt: polymorphic equality dispatch using protocol-only implicit evidence.
def Stigi.sameValue [A: Type] {Stigi.Equal[A]} (left: A, right: A): Bool =
  Stigi.Equal.equal(left, right)
