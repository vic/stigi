// prompt: match an atom against a stored tuple row, binding unbound variables and recording their slot mask.
def Stigi.Datalog.Engine.bindAtom {Stigi.Mut} (varCap: U64, varBound: Array[Bool](varCap), varVals: Array[Stigi.Datalog.Store.Cell](varCap), atom: Stigi.Datalog.Check.Atom, row: Stigi.Datalog.Store.Tuple) <below(0, varCap)>: Stigi.Datalog.Engine.BindOutcome =
  atom match
    case AtomRef(atomRel, arity, _, _, _, _, _, _, _) =>
      row match
        case StoreRow(rowRel, rowArity, _, _, _, _, _, _, _) =>
          if atomRel != rowRel || arity != rowArity then
            BindFailed
          else
            var slot: U64 = 0
            var newMask: U64 = 0
            var failedFlag: Bool = false
            while slot < arity && !failedFlag do
              @<lessOrEqual(slot, arity)>
              val nextSlot: U64 = slot + 1
              val argTerm: Stigi.Datalog.Check.Term = Stigi.Datalog.Check.atomArg(atom, slot)
              val cellVal: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.rowCol(row, slot)
              argTerm match
                case ConstSym(symNum) =>
                  if !Stigi.Datalog.Store.sameCell(SymCell(symNum), cellVal) then
                    failedFlag = true
                  else
                    ()
                case ConstNum(lowWord, highWord) =>
                  if !Stigi.Datalog.Store.sameCell(NumCell(WideInt(lowWord, highWord)), cellVal) then
                    failedFlag = true
                  else
                    ()
                case VarTerm(varNum) =>
                  if varNum < varCap then
                    if varBound(varNum) then
                      if !Stigi.Datalog.Store.sameCell(varVals(varNum), cellVal) then
                        failedFlag = true
                      else
                        ()
                    else
                      varBound(varNum) := true
                      varVals(varNum) := cellVal
                      newMask = U64.bitwiseOr(newMask, U64.shiftLeft(1, slot))
                  else
                    failedFlag = true
              slot = nextSlot
            if failedFlag then
              Stigi.Datalog.Engine.unbindAtom(varCap, varBound, atom, newMask)
              BindFailed
            else
              BindMatched(newMask)
