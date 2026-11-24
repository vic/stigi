// prompt: find the newest of the first locals of a walker whose text equals a source run and give the capability it carries.
def Stigi.Names.localBinding(tables: Stigi.Syntax.Tables, locals: Array[Stigi.Names.Local](Stigi.Names.maxScope), top: U64, first: U64, end: U64, remaining: U64) <lessOrEqual(top, Stigi.Names.maxScope), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Names.LocalLookup =
  if remaining < 1 || top < 1 then NotLocal else
    val Local(text, binding) = locals(top - 1)
    val Text(localFirst, localEnd) = text
    if Stigi.Names.sameRun(tables, localFirst, localEnd, first, end, localEnd - localFirst + 1) then LocalBound(binding) else Stigi.Names.localBinding(tables, locals, top - 1, first, end, remaining - 1)
