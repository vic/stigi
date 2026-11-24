// prompt: parse a def, lemma def or extern def from its name: its head, then its clauses or body after the equals sign.
def Stigi.Syntax.Parse.parseFunction {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool, kind: Stigi.Syntax.Parse.FunctionKind, column: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if depth < 1 then Failed(TooDeep, token) else if !Stigi.Syntax.Parse.isWord(tables, token) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readHead(tables, Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(head, afterHead) =>
        val Progress(equals, headNodes, headLists, headPending) = afterHead
        val next: U64 = Stigi.Syntax.Parse.nextToken(equals)
        val bodyStart: Stigi.Syntax.Parse.Progress = Progress(next, headNodes, headLists, headPending)
        val hasBody: Bool = Stigi.Syntax.Parse.kindAt(tables, equals) is TokAssign
        val armed: Bool = hasBody && Stigi.Syntax.Parse.kindAt(tables, next) is TokNewline && Stigi.Syntax.Parse.kindAt(tables, Stigi.Syntax.Parse.lineStart(tables, next, Stigi.Limit.maxUnitBytes)) is TokCase
        val clauses: Stigi.Syntax.Parse.Listed = if armed then Stigi.Syntax.Parse.parseArms(tables, bodyStart, depth - 1) else Stigi.Syntax.Parse.emptyList(afterHead)
        clauses match
          case ListFailed(fault, at) => Failed(fault, at)
          case Listed(arms, afterArms) => Stigi.Syntax.Parse.joinFunction(tables, isPublic, kind, Stigi.Syntax.Parse.spanText(tables, token, last), head, arms, if hasBody && !armed then Stigi.Syntax.Parse.parseBody(tables, bodyStart, column, depth - 1) else Stigi.Syntax.Parse.absent(tables, afterArms))
