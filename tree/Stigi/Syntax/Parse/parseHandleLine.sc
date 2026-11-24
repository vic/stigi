// prompt: parse a handle statement after its head: the value arguments and an optional body.
def Stigi.Syntax.Parse.parseHandleLine {Stigi.Mut} (tables: Stigi.Syntax.Tables, head: Stigi.Syntax.Parse.Head, column: U64, token: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  if depth < 1 then Failed(TooDeep, token) else
    head match
      case HandleHead(_, _, call) =>
        call match
          case CallHeadFailed(fault, at) => Failed(fault, at)
          case CallHead(_, _, parens, afterHead) =>
            val Progress(argToken, argNodes, argLists, argPending) = afterHead
            val args: Stigi.Syntax.Parse.Listed = if parens && argToken < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.parseArgs(tables, Progress(argToken + 1, argNodes, argLists, argPending), Parens, column, depth - 1)) else Stigi.Syntax.Parse.emptyList(afterHead)
            args match
              case ListFailed(fault, at) => Failed(fault, at)
              case Listed(values, afterArgs) =>
                val Progress(colon, colonNodes, colonLists, colonPending) = afterArgs
                Stigi.Syntax.Parse.joinHandle(tables, head, values, if Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon && colon < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.parseBody(tables, Progress(colon + 1, colonNodes, colonLists, colonPending), column, depth - 1) else Stigi.Syntax.Parse.absent(tables, afterArgs), token)
      case ValHead(_, _, _) | VarHead(_, _, _, _) | AssignHead(_, _) | ValueHead(_) | HeadFailed(_, _) => Failed(UnexpectedToken, token)
