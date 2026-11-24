// prompt: read the head of a handle statement: an optional binder, the handler name and its type and capability arguments.
def Stigi.Syntax.Parse.readHandleHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Head =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then HeadFailed(UnexpectedToken, token) else
    val bound: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.isWord(tables, token + 1) && Stigi.Syntax.Parse.kindAt(tables, token + 2) is TokAssign then Stigi.Syntax.Parse.parseName(tables, Progress(token + 1, nodes, lists, pending)) else Stigi.Syntax.Parse.absent(tables, Progress(token + 1, nodes, lists, pending))
    bound match
      case Failed(fault, at) => HeadFailed(fault, at)
      case Built(binder, afterBinder) =>
        val Progress(binderEnd, binderNodes, binderLists, binderPending) = afterBinder
        val nameToken: U64 = if Stigi.Syntax.Parse.kindAt(tables, binderEnd) is TokAssign && binderEnd < Stigi.Limit.maxUnitBytes then binderEnd + 1 else binderEnd
        val last: U64 = Stigi.Syntax.Parse.nameLast(tables, nameToken, Stigi.Limit.maxUnitBytes)
        if !Stigi.Syntax.Parse.isWord(tables, nameToken) || Stigi.Limit.maxUnitBytes <= last then HeadFailed(UnexpectedToken, nameToken) else HandleHead(binder, Stigi.Syntax.Parse.spanText(tables, nameToken, last), Stigi.Syntax.Parse.readCallHead(tables, Progress(last + 1, binderNodes, binderLists, binderPending)))
