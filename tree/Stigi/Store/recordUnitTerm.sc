// prompt: normalize one length expression syntax node, append lengthTerm and linearForm rows, and record its normal form.
def Stigi.Store.recordUnitTerm {region: Stigi.Region, Stigi.Mut} (synTables: Stigi.Syntax.Tables, image: Stigi.Names.Image, unitIdx: U64, lenSyn: Stigi.Syntax.NodeAt, state: Stigi.Store.LengthUnitState) <below(unitIdx, Stigi.Limit.maxUnits)>: Stigi.Store.StepUnitOutcome =
  val Image(namePool, entries, count, _) = image
  if count <= unitIdx then StepUnitStopped(FactsOutOfMemory) else
    val Entry(nameFirst, nameEnd, _, _) = entries(unitIdx)
    val unitSlice: Stigi.Store.NameSlice = NameSlice(namePool, nameFirst, nameEnd)
    state match
      case LengthUnitState(scratch, localCount, termCount, poolEnd, workTop, lanes) =>
        scratch match
          case LengthScratch(pathBuf, sourceBuf, unitSyn, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack) =>
            if 256 <= termCount then StepUnitDone(state) else
              lanes match
                case LengthLanes(lengthLane, linearLane, offsetLane, sameLane, rowCount) =>
                  if Stigi.Limit.maxEmitBytes <= rowCount then StepUnitStopped(FactsStopped(TypeLaneFull(LengthTermFile))) else
                    val LaneWritten(nextLengthLane) = Stigi.Store.putLengthAtomLane{region}(lengthLane, unitSlice, termCount) else
                      case LaneOverflow => StepUnitStopped(FactsStopped(TypeLaneFull(LengthTermFile)))
                    val lowered: Stigi.Elaborate.LowerOutcome = Stigi.Elaborate.lowerSyntaxNode(synTables, lenSyn, coreTables, elabScratch)
                    val nextLinearRes: Stigi.Types.LengthOutcome = lowered match
                      case LowerOverflow | LowerInvalid => LengthNotLinear
                      case Lowered(rootCore, loweredCore, _) =>
                        val CoreTables(_, _, _, exprCount, _, _, _, _, _, _, _, _) = loweredCore
                        if exprCount <= Stigi.Limit.maxUnitNodes && rootCore < Stigi.Limit.maxUnitNodes then
                          Stigi.Store.resolveLengthRefs(synTables, image, unitIdx, scratch, localCount, exprCount)
                          Stigi.Types.lengthOf(loweredCore, LengthSpace(1024, pool, poolEnd, 256, binders, Stigi.Limit.maxUnits, globals, nodeStack, stepStack, formStack), rootCore)
                        else LengthNotLinear
                    val nextTerms: U64 = termCount + 1
                    nextLinearRes match
                      case LengthValue(normForm, nextPool) =>
                        val LaneWritten(nextLinearLane) = Stigi.Store.putLengthAtomLane{region}(linearLane, unitSlice, termCount) else
                          case LaneOverflow => StepUnitStopped(FactsStopped(TypeLaneFull(LinearFormFile)))
                        unitForms(termCount) := normForm
                        unitValid(termCount) := true
                        val keptPool: U64 = if nextPool <= 1024 then nextPool else poolEnd
                        val nextScratch: Stigi.Store.LengthScratch = LengthScratch(pathBuf, sourceBuf, unitSyn, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack)
                        StepUnitDone(LengthUnitState(nextScratch, localCount, nextTerms, keptPool, workTop, LengthLanes(nextLengthLane, nextLinearLane, offsetLane, sameLane, rowCount + 2)))
                      case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned =>
                        unitValid(termCount) := false
                        val nextScratch: Stigi.Store.LengthScratch = LengthScratch(pathBuf, sourceBuf, unitSyn, coreTables, elabScratch, work, pool, localStarts, localEnds, binders, globals, unitForms, unitValid, nodeStack, stepStack, formStack)
                        StepUnitDone(LengthUnitState(nextScratch, localCount, nextTerms, poolEnd, workTop, LengthLanes(nextLengthLane, linearLane, offsetLane, sameLane, rowCount + 1)))
