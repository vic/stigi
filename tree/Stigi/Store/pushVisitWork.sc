// prompt: push one VisitWork item onto the unit traversal work stack when capacity remains.
def Stigi.Store.pushVisitWork {Stigi.Mut} (unitState: Stigi.Store.LengthUnitState, item: Stigi.Store.VisitWork): Stigi.Store.LengthUnitState =
  unitState match
    case LengthUnitState(scratch, localCount, termCount, poolEnd, workTop, lanes) =>
      scratch match
        case LengthScratch(pathBuf, sourceBuf, synTables, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack) =>
          if workTop < 4096 then
            @<below(workTop, 4096)>
            work(workTop) := item
            LengthUnitState(LengthScratch(pathBuf, sourceBuf, synTables, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack), localCount, termCount, poolEnd, workTop + 1, lanes)
          else unitState
