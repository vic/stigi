// prompt: whether one of the first locals of a walker, scanned from the newest down, has the text of a source run.
def Stigi.Names.localAt(tables: Stigi.Syntax.Tables, locals: Array[Stigi.Names.Local](Stigi.Names.maxScope), top: U64, first: U64, end: U64, remaining: U64) <lessOrEqual(top, Stigi.Names.maxScope), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 || top < 1 then false else
    val Local(text, _) = locals(top - 1)
    val Text(localFirst, localEnd) = text
    if Stigi.Names.sameRun(tables, localFirst, localEnd, first, end, localEnd - localFirst + 1) then true else Stigi.Names.localAt(tables, locals, top - 1, first, end, remaining - 1)
