// prompt: read a rule def from its name token: its name, its head fact and the body facts on its line or on the next line.
def Stigi.Syntax.Parse.readRule {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  val colon: U64 = Stigi.Syntax.Parse.nextToken(last)
  if !Stigi.Syntax.Parse.isWord(tables, token) || !(Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readFact(tables, Progress(Stigi.Syntax.Parse.nextToken(colon), nodes, lists, pending)) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(head, afterHead) =>
        val Progress(equals, headNodes, headLists, headPending) = afterHead
        val next: U64 = Stigi.Syntax.Parse.nextToken(equals)
        val hasBody: Bool = Stigi.Syntax.Parse.kindAt(tables, equals) is TokAssign
        val multiline: Bool = hasBody && Stigi.Syntax.Parse.kindAt(tables, next) is TokNewline
        val first: U64 = if multiline then Stigi.Syntax.Parse.lineStart(tables, next, Stigi.Limit.maxUnitBytes) else next
        val bodied: Stigi.Syntax.Parse.Listed = if hasBody then Stigi.Syntax.Parse.readFacts(tables, Progress(first, headNodes, headLists, headPending), LineEnd) else Stigi.Syntax.Parse.emptyList(afterHead)
        bodied match
          case ListFailed(fault, at) => Failed(fault, at)
          case Listed(body, after) =>
            Stigi.Syntax.Parse.claimOver(after, head, head, head) match
              case Unclaimed(fault) => Failed(fault, token)
              case Claimed(at, headed, _, _) => Stigi.Syntax.Parse.place(tables, at, RuleDecl(at, isPublic, Stigi.Syntax.Parse.spanText(tables, token, last), headed, body, multiline), after)
