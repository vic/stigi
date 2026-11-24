// prompt: verify that a Datalog atom references a declared relation with matching arity.
def Stigi.Datalog.Check.checkAtomRef(relCap: U64, relArities: Array[U64](relCap), relCount: U64, ruleIndex: U64, atom: Stigi.Datalog.Check.Atom) <lessOrEqual(relCount, relCap)>: Stigi.Datalog.Check.Outcome =
  atom match
    case AtomRef(relNum, arity, _, _, _, _, _, _, _) =>
      if relNum < relCount then
        if arity == relArities(relNum) then
          CheckPassed(0)
        else
          ArityMismatch(ruleIndex, relNum)
      else
        UnresolvedRelation(ruleIndex, relNum)
