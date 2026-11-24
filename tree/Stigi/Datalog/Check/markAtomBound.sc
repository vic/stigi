// prompt: mark all active variable arguments of a positive body atom as bound.
def Stigi.Datalog.Check.markAtomBound {Stigi.Mut} (varCap: U64, bound: Array[Bool](varCap), atom: Stigi.Datalog.Check.Atom) <below(0, varCap)>: Unit =
  atom match
    case AtomRef(_, arity, _, _, _, _, _, _, _) =>
      var slot: U64 = 0
      while slot < arity do
        @<lessOrEqual(slot, arity)>
        val nextSlot: U64 = slot + 1
        val argTerm: Stigi.Datalog.Check.Term = Stigi.Datalog.Check.atomArg(atom, slot)
        Stigi.Datalog.Check.markTermBound(varCap, bound, argTerm)
        slot = nextSlot
