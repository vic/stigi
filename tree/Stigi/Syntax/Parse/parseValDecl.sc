// prompt: parse a val declaration from its name token: its head, then its body.
def Stigi.Syntax.Parse.parseValDecl {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool, column: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if depth < 1 then Failed(TooDeep, token) else
    val head: Stigi.Syntax.Parse.ValDeclHead = Stigi.Syntax.Parse.readValDeclHead(tables, start)
    head match
      case ValDeclHeadFailed(fault, at) => Failed(fault, at)
      case ValDeclHead(_, _, _, afterHead) => Stigi.Syntax.Parse.joinValDecl(tables, isPublic, head, Stigi.Syntax.Parse.parseBody(tables, afterHead, column, depth - 1), token)
