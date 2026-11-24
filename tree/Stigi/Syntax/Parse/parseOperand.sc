// prompt: parse an operand: a prefix operator and its operand, or a primary expression followed by calls and assertions.
def Stigi.Syntax.Parse.parseOperand {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if depth < 1 then Failed(TooDeep, token) else if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    val primary: Stigi.Syntax.Parse.Built = Stigi.Syntax.Parse.leadOf(Stigi.Syntax.Parse.kindAt(tables, token)) match
      case StartsPrefix(operator) => Stigi.Syntax.Parse.joinUnary(tables, operator, Stigi.Syntax.Parse.parseOperand(tables, Progress(token + 1, nodes, lists, pending), indent, depth - 1), token)
      case StartsLiteral(kind) => Stigi.Syntax.Parse.literal(tables, start, kind, token)
      case StartsName => Stigi.Syntax.Parse.reference(tables, start)
      case StartsAssert => Stigi.Syntax.Parse.bareAssert(tables, start)
      case StartsParen =>
        if Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokRightParen then Stigi.Syntax.Parse.literal(tables, start, UnitLiteral, token + 1) else if Stigi.Syntax.Parse.scanTuple(tables, token + 1, 1, false, Stigi.Limit.maxUnitBytes) then Stigi.Syntax.Parse.parseControl(tables, start, indent, depth - 1) else Stigi.Syntax.Parse.closeParen(tables, Stigi.Syntax.Parse.parseExpr(tables, Progress(token + 1, nodes, lists, pending), 0, indent, depth - 1))
      case StartsIf | StartsWhile => Stigi.Syntax.Parse.parseControl(tables, start, indent, depth - 1)
      case StartsVal | StartsVar | StartsHandle | StartsDecl | StartsNothing => Failed(UnexpectedToken, token)
    var state: Stigi.Syntax.Parse.Built = primary
    var steps: U64 = 0
    while steps < Stigi.Limit.maxUnitBytes && Stigi.Syntax.Parse.continuesPostfix(tables, state) do
      state = state match
        case Failed(fault, at) => Failed(fault, at)
        case Built(left, progress) =>
          val Progress(postToken, postNodes, postLists, postPending) = progress
          if Stigi.Limit.maxUnitBytes <= postToken then Failed(UnexpectedToken, postToken) else if Stigi.Syntax.Parse.kindAt(tables, postToken) is TokAt then Stigi.Syntax.Parse.joinAssert(tables, left, Stigi.Syntax.Parse.readFactGroup(tables, Progress(postToken + 1, postNodes, postLists, postPending)), postToken) else
            val head: Stigi.Syntax.Parse.CallHead = Stigi.Syntax.Parse.readCallHead(tables, progress)
            val args: Stigi.Syntax.Parse.Listed = head match
              case CallHeadFailed(fault, at) => ListFailed(fault, at)
              case CallHead(_, _, parens, afterHead) =>
                val Progress(argToken, argNodes, argLists, argPending) = afterHead
                if parens && argToken < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.parseArgs(tables, Progress(argToken + 1, argNodes, argLists, argPending), Parens, indent, depth - 1)) else Stigi.Syntax.Parse.emptyList(afterHead)
            Stigi.Syntax.Parse.joinCall(tables, left, head, args, postToken)
      steps = steps + 1
    state
