// prompt: parse one statement line at a column: a local declaration, or a statement from its head.
def Stigi.Syntax.Parse.parseLine {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, column: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(first, _, _, _) = start
  if depth < 1 then Failed(TooDeep, first) else if Stigi.Syntax.Parse.leadOf(Stigi.Syntax.Parse.kindAt(tables, first)) is StartsDecl then Stigi.Syntax.Parse.joinLocal(tables, Stigi.Syntax.Parse.parseDecl(tables, start, column, depth - 1), first) else
    val head: Stigi.Syntax.Parse.Head = Stigi.Syntax.Parse.statementHead(tables, start)
    head match
      case HeadFailed(fault, at) => Failed(fault, at)
      case ValHead(_, _, afterHead) =>
        Stigi.Syntax.Parse.parseBody(tables, afterHead, column, depth - 1) match
          case Failed(fault, at) => Failed(fault, at)
          case Built(init, afterInit) =>
            val Progress(elseToken, initNodes, initLists, initPending) = afterInit
            Stigi.Syntax.Parse.joinVal(tables, head, init, if Stigi.Syntax.Parse.kindAt(tables, elseToken) is TokElse && elseToken < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.parseArms(tables, Progress(elseToken + 1, initNodes, initLists, initPending), depth - 1) else Stigi.Syntax.Parse.emptyList(afterInit), first)
      case VarHead(_, _, _, afterHead) => Stigi.Syntax.Parse.joinVar(tables, head, Stigi.Syntax.Parse.parseBody(tables, afterHead, column, depth - 1), first)
      case AssignHead(_, afterHead) => Stigi.Syntax.Parse.joinAssign(tables, head, Stigi.Syntax.Parse.parseBody(tables, afterHead, column, depth - 1), first)
      case HandleHead(_, _, _) => Stigi.Syntax.Parse.parseHandleLine(tables, head, column, first, depth - 1)
      case ValueHead(afterHead) =>
        Stigi.Syntax.Parse.parseExpr(tables, afterHead, 0, column, depth - 1) match
          case Failed(fault, at) => Failed(fault, at)
          case Built(value, afterValue) =>
            val Progress(writeToken, writeNodes, writeLists, writePending) = afterValue
            if Stigi.Limit.maxUnitBytes <= writeToken || !(Stigi.Syntax.Parse.kindAt(tables, writeToken) is TokColonAssign) then Built(value, afterValue) else Stigi.Syntax.Parse.joinIndexWrite(tables, value, Stigi.Syntax.Parse.parseBody(tables, Progress(writeToken + 1, writeNodes, writeLists, writePending), column, depth - 1), first)
