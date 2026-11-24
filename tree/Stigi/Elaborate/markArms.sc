// prompt: give back the core tables with the arm mark moved to the arm fill end, so the arms appended next form one span.
def Stigi.Elaborate.markArms(core: Stigi.Core.Tables): Stigi.Core.Tables =
  core match
    case CoreTables(expressionCapacity, exprs, expressionMark, expressionEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, _, armEnd) =>
      @<lessOrEqual(armEnd, armCapacity)>
      CoreTables(expressionCapacity, exprs, expressionMark, expressionEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armEnd, armEnd)
