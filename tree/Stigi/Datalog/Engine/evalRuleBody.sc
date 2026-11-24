// prompt: evaluate one Datalog rule body iteratively with backtracking, checked arithmetic, and semi-naive delta filtering.
def Stigi.Datalog.Engine.evalRuleBody {Stigi.Mut} (relCap: U64, rowCap: U64, tupleSlotCap: U64, litCap: U64, varCap: U64, symCap: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, rows: Array[Stigi.Datalog.Store.Tuple](rowCap), tupleSlots: Array[Stigi.Datalog.Store.Slot](tupleSlotCap), relHead: Array[Stigi.Datalog.Store.Slot](relCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), varBound: Array[Bool](varCap), varVals: Array[Stigi.Datalog.Store.Cell](varCap), ranks: Array[U64](symCap), iterSlot: Array[Stigi.Datalog.Store.Slot](litCap), iterMask: Array[U64](litCap), ruleItem: Stigi.Datalog.Check.RuleDef, rowCount: U64, derivedCount: U64, useDelta: Bool, deltaLitIdx: U64, deltaStart: U64, deltaEnd: U64) <below(0, tupleSlotCap), below(0, varCap), below(0, symCap), lessOrEqual(litCount, litCap), lessOrEqual(rowCount, rowCap)>: Stigi.Datalog.Engine.StepState =
  var clrIdx: U64 = 0
  while clrIdx < varCap do
    @<lessOrEqual(clrIdx, varCap)>
    val nextClr: U64 = clrIdx + 1
    varBound(clrIdx) := false
    clrIdx = nextClr
  ruleItem match
    case RuleEntry(head, litStart, litEnd) =>
      if litEnd > litCount then
        StepDone(rowCount, derivedCount)
      else
        var curLit: U64 = litStart
        var entering: Bool = true
        var doneLoop: Bool = false
        var curRowCount: U64 = rowCount
        var curDerived: U64 = derivedCount
        var tableFull: Bool = false
        var arithFailed: Bool = false
        while !doneLoop && !tableFull && !arithFailed do
          if curLit == litEnd then
            val headRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.materializeAtom(varCap, varVals, head)
            if curRowCount <= rowCap then
              Stigi.Datalog.Store.insertTuple(relCap, rowCap, tupleSlotCap, rows, tupleSlots, relHead, nextInRel, headRow, curRowCount) match
                case TupleAdded(_, nextCount) =>
                  curRowCount = nextCount
                  curDerived = U64.wrappingAdd(curDerived, 1)
                case TupleDuplicate(_) => ()
                case TupleTableFull => tableFull = true
            else
              tableFull = true
            if curLit > litStart then
              curLit = curLit - 1
              entering = false
            else
              doneLoop = true
          else if curLit < litEnd then
            val bodyLit: Stigi.Datalog.Check.Lit = lits(curLit)
            bodyLit match
              case CmpLit(kind, leftTerm, rightTerm) =>
                if entering then
                  val leftCell: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, leftTerm)
                  val rightCell: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, rightTerm)
                  if Stigi.Datalog.Engine.evalCompare(kind, leftCell, rightCell, symCap, ranks) then
                    curLit = curLit + 1
                  else if curLit > litStart then
                    curLit = curLit - 1
                    entering = false
                  else
                    doneLoop = true
                else if curLit > litStart then
                  curLit = curLit - 1
                else
                  doneLoop = true
              case ArithLit(kind, dstVar, leftTerm, rightTerm) =>
                if entering then
                  Stigi.Datalog.Engine.evalArith(varCap, varVals, kind, leftTerm, rightTerm) match
                    case ArithErr =>
                      arithFailed = true
                    case ArithVal(resCell) =>
                      if dstVar < varCap then
                        if varBound(dstVar) then
                          iterMask(curLit) := 0
                          if Stigi.Datalog.Store.sameCell(varVals(dstVar), resCell) then
                            curLit = curLit + 1
                          else if curLit > litStart then
                            curLit = curLit - 1
                            entering = false
                          else
                            doneLoop = true
                        else
                          varBound(dstVar) := true
                          varVals(dstVar) := resCell
                          iterMask(curLit) := 1
                          curLit = curLit + 1
                      else if curLit > litStart then
                        curLit = curLit - 1
                        entering = false
                      else
                        doneLoop = true
                else
                  if iterMask(curLit) != 0 && dstVar < varCap then
                    varBound(dstVar) := false
                    iterMask(curLit) := 0
                  else
                    ()
                  if curLit > litStart then
                    curLit = curLit - 1
                  else
                    doneLoop = true
              case NegLit(negAtom) =>
                if entering then
                  val negRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.materializeAtom(varCap, varVals, negAtom)
                  val present: Bool = if curRowCount <= rowCap then Stigi.Datalog.Store.hasTuple(rowCap, tupleSlotCap, rows, tupleSlots, negRow, curRowCount) else false
                  if !present then
                    curLit = curLit + 1
                  else if curLit > litStart then
                    curLit = curLit - 1
                    entering = false
                  else
                    doneLoop = true
                else if curLit > litStart then
                  curLit = curLit - 1
                else
                  doneLoop = true
              case PosLit(posAtom) =>
                if entering then
                  posAtom match
                    case AtomRef(bodyRel, _, _, _, _, _, _, _, _) =>
                      if bodyRel < relCap then
                        iterSlot(curLit) := relHead(bodyRel)
                      else
                        iterSlot(curLit) := StoreVacant
                  iterMask(curLit) := 0
                else
                  Stigi.Datalog.Engine.unbindAtom(varCap, varBound, posAtom, iterMask(curLit))
                  iterMask(curLit) := 0
                var scanSlot: Stigi.Datalog.Store.Slot = iterSlot(curLit)
                var scanSteps: U64 = 0
                var matched: Bool = false
                while !matched && scanSteps < rowCap do
                  val nextSteps: U64 = scanSteps + 1
                  scanSlot match
                    case StoreVacant =>
                      scanSteps = rowCap
                    case StoreOccupied(candRow) =>
                      val nextSlot: Stigi.Datalog.Store.Slot = if candRow < rowCap then nextInRel(candRow) else StoreVacant
                      val inRange: Bool = if useDelta && curLit == deltaLitIdx then candRow >= deltaStart && candRow < deltaEnd else candRow < deltaEnd
                      if inRange && candRow < rowCap then
                        Stigi.Datalog.Engine.bindAtom(varCap, varBound, varVals, posAtom, rows(candRow)) match
                          case BindMatched(newMask) =>
                            iterSlot(curLit) := nextSlot
                            iterMask(curLit) := newMask
                            matched = true
                          case BindFailed =>
                            scanSlot = nextSlot
                      else
                        scanSlot = nextSlot
                      scanSteps = nextSteps
                if matched then
                  curLit = curLit + 1
                  entering = true
                else if curLit > litStart then
                  curLit = curLit - 1
                  entering = false
                else
                  doneLoop = true
          else
            doneLoop = true
        if arithFailed then
          StepArithError
        else if tableFull then
          StepFull
        else
          StepDone(curRowCount, curDerived)
