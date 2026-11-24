// prompt: print a pattern node (PatName, PatLiteral, PatCtor, PatTuple, PatOr, PatRange, or Absent), matching parser hop counts including inline PatOr.
def Stigi.Syntax.Print.printPattern {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else
    Stigi.Syntax.readSlot(tables, node) match
      case MalformedTable(slot) => PenFailed(MalformedNode(slot))
      case Fetched(found) =>
        found match
          case Absent(_) => pen
          case PatName(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
          case PatLiteral(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
          case PatCtor(_, name, parts) => Stigi.Syntax.Print.printPatternList(tables, parts, InDelimiters(InParens), Stigi.Syntax.Print.putText(tables, pen, name), depth - 1)
          case PatTuple(_, parts) => Stigi.Syntax.Print.printPatternList(tables, parts, InDelimiters(InParens), pen, depth - 1)
          case PatOr(_, alternatives) =>
            val Span(first, endPos) = alternatives
            val Tables(_, _, _, lists, _, _) = tables
            var current: Stigi.Syntax.Print.Pen = pen
            var index: U64 = first
            while index < endPos && current is Pen(_) do
              @<lessOrEqual(index, endPos)>
              val item: U64 = lists(index)
              val led: Stigi.Syntax.Print.Pen = if first < index then Stigi.Syntax.Print.putLiteral(tables, current, 3, " | ") else current
              current = if item < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printPattern(tables, item, led, depth - 1) else PenFailed(MalformedNode(index))
              index = index + 1
            current
          case PatRange(_, low, high) => Stigi.Syntax.Print.printPattern(tables, high, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printPattern(tables, low, pen, depth - 1), 2, ".."), depth - 1)
          case Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
