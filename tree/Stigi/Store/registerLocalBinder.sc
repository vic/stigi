// prompt: register one local binder slice and kind into the unit binder table, returning the updated unitState.
def Stigi.Store.registerLocalBinder {Stigi.Mut} (unitState: Stigi.Store.LengthUnitState, nameFirst: U64, nameEnd: U64, kind: Stigi.Types.BinderKind): Stigi.Store.LengthUnitState =
  unitState match
    case LengthUnitState(scratch, localCount, termCount, poolEnd, workTop, lanes) =>
      scratch match
        case LengthScratch(pathBuf, sourceBuf, synTables, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack) =>
          if localCount < 255 then
            @<below(localCount, 255)>
            localStarts(localCount) := nameFirst
            localEnds(localCount) := nameEnd
            binders(localCount) := kind
            LengthUnitState(LengthScratch(pathBuf, sourceBuf, synTables, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack), localCount + 1, termCount, poolEnd, workTop, lanes)
          else unitState
