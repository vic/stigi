// prompt: record all length arguments of one TypeRef node and schedule its type arguments for traversal.
def Stigi.Store.processTypeRef {region: Stigi.Region, Stigi.Mut} (synTables: Stigi.Syntax.Tables, image: Stigi.Names.Image, unitIdx: U64, state: Stigi.Store.LengthUnitState, typeArgs: Stigi.Syntax.Span, lengthArgs: Stigi.Syntax.Span) <below(unitIdx, Stigi.Limit.maxUnits)>: Stigi.Store.StepUnitOutcome =
  val Tables(_, _, _, synLists, _, _) = synTables
  var nextState: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, state, typeArgs, true)
  var stopped: Bool = false
  var fault: Stigi.Store.FactsFailure = FactsOutOfMemory
  lengthArgs match
    case Span(lenFirst, lenEnd) =>
      var pos: U64 = lenFirst
      while pos < lenEnd && !stopped do
        @<lessOrEqual(lenFirst, pos), lessOrEqual(pos, lenEnd)>
        val lenSyn: Stigi.Syntax.NodeAt = synLists(pos)
        Stigi.Store.recordUnitTerm{region}(synTables, image, unitIdx, lenSyn, nextState) match
          case StepUnitDone(updated) => nextState = updated
          case StepUnitStopped(why) =>
            stopped = true
            fault = why
        pos = pos + 1
  if stopped then StepUnitStopped(fault) else StepUnitDone(nextState)
