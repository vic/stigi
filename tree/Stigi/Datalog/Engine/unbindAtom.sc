// prompt: unbind variables that were newly bound by an atom match according to its 7-bit slot mask.
def Stigi.Datalog.Engine.unbindAtom {Stigi.Mut} (varCap: U64, varBound: Array[Bool](varCap), atom: Stigi.Datalog.Check.Atom, boundMask: U64) <below(0, varCap)>: Unit =
  atom match
    case AtomRef(_, arity, _, _, _, _, _, _, _) =>
      var slot: U64 = 0
      while slot < arity do
        @<lessOrEqual(slot, arity)>
        val nextSlot: U64 = slot + 1
        val bitFlag: U64 = U64.bitwiseAnd(U64.shiftRight(boundMask, slot), 1)
        if bitFlag != 0 then
          Stigi.Datalog.Check.atomArg(atom, slot) match
            case VarTerm(varNum) =>
              if varNum < varCap then
                varBound(varNum) := false
              else
                ()
            case ConstSym(_) => ()
            case ConstNum(_, _) => ()
        else
          ()
        slot = nextSlot
