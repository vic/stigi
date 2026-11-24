// prompt: check that every active variable argument of an atom is bound in the safety table.
def Stigi.Datalog.Check.checkAtomBound(varCap: U64, bound: Array[Bool](varCap), ruleIndex: U64, atom: Stigi.Datalog.Check.Atom) <below(0, varCap)>: Stigi.Datalog.Check.Outcome =
  atom match
    case AtomRef(_, arity, _, _, _, _, _, _, _) =>
      var slot: U64 = 0
      var verdict: Stigi.Datalog.Check.Outcome = CheckPassed(0)
      var foundBad: Bool = false
      while slot < arity && !foundBad do
        @<lessOrEqual(slot, arity)>
        val nextSlot: U64 = slot + 1
        val argTerm: Stigi.Datalog.Check.Term = Stigi.Datalog.Check.atomArg(atom, slot)
        verdict = Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, argTerm)
        foundBad = !Stigi.Datalog.Check.isPassed(verdict)
        slot = nextSlot
      verdict
