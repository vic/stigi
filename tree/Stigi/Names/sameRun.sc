// prompt: whether two runs of source bytes hold the same bytes.
def Stigi.Names.sameRun(tables: Stigi.Syntax.Tables, left: U64, leftEnd: U64, right: U64, rightEnd: U64, remaining: U64) <lessOrEqual(leftEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(rightEnd, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 then false else
    if leftEnd <= left then rightEnd <= right else
      if rightEnd <= right then false else
        val Tables(source, _, _, _, _, _) = tables
        if source(left) == source(right) then Stigi.Names.sameRun(tables, left + 1, leftEnd, right + 1, rightEnd, remaining - 1) else false
