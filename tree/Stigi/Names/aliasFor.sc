// prompt: scan the locals of a walker from the oldest and give the target of the first alias whose name equals the whole reference or starts it before a dot, with the tail of the reference that remains.
def Stigi.Names.aliasFor(tables: Stigi.Syntax.Tables, locals: Array[Stigi.Names.Local](Stigi.Names.maxScope), localCount: U64, first: U64, end: U64, at: U64, remaining: U64) <lessOrEqual(localCount, Stigi.Names.maxScope), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Names.AliasHit =
  if remaining < 1 || localCount <= at then Unaliased else
    locals(at) match
      case Local(_, _) => Stigi.Names.aliasFor(tables, locals, localCount, first, end, at + 1, remaining - 1)
      case AliasLocal(name, targetFirst, targetEnd) =>
        val Text(nameFirst, nameEnd) = name
        if Stigi.Names.sameRun(tables, nameFirst, nameEnd, first, end, nameEnd - nameFirst + 1) then AliasHit(Text(targetFirst, targetEnd), Text(end, end)) else
          val cut: U64 = first + (nameEnd - nameFirst)
          if cut < end then
            val Tables(source, _, _, _, _, _) = tables
            if source(cut) == '.' then
              if Stigi.Names.sameRun(tables, nameFirst, nameEnd, first, cut, nameEnd - nameFirst + 1) then AliasHit(Text(targetFirst, targetEnd), Text(cut, end)) else Stigi.Names.aliasFor(tables, locals, localCount, first, end, at + 1, remaining - 1)
            else Stigi.Names.aliasFor(tables, locals, localCount, first, end, at + 1, remaining - 1)
          else Stigi.Names.aliasFor(tables, locals, localCount, first, end, at + 1, remaining - 1)
