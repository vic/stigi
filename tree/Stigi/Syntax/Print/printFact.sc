// prompt: print a fact, denial, or fact group iteratively without node-depth recursion.
def Stigi.Syntax.Print.printFact {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  Stigi.Syntax.readSlot(tables, node) match
    case MalformedTable(slot) => PenFailed(MalformedNode(slot))
    case Fetched(found) =>
      val Tables(_, _, _, lists, _, _) = tables
      found match
        case Absent(_) => pen
        case FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) => Stigi.Syntax.Print.printPlainFact(tables, node, pen)
        case FactDeny(_, items) =>
          val Span(first, endPos) = items
          var current: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, pen, 5, "not (")
          var index: U64 = first
          while index < endPos && current is Pen(_) do
            @<lessOrEqual(index, endPos)>
            val item: U64 = lists(index)
            val separated: Stigi.Syntax.Print.Pen = if first < index then Stigi.Syntax.Print.putLiteral(tables, current, 2, ", ") else current
            current = if item < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printPlainFact(tables, item, separated) else PenFailed(MalformedNode(index))
            index = index + 1
          Stigi.Syntax.Print.putLiteral(tables, current, 1, ")")
        case FactGroup(_, items) =>
          val Span(first, endPos) = items
          var current: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, pen, 1, "<")
          var index: U64 = first
          while index < endPos && current is Pen(_) do
            @<lessOrEqual(index, endPos)>
            val item: U64 = lists(index)
            val separated: Stigi.Syntax.Print.Pen = if first < index then Stigi.Syntax.Print.putLiteral(tables, current, 2, ", ") else current
            current = if item < Stigi.Limit.maxUnitNodes then
              Stigi.Syntax.readSlot(tables, item) match
                case MalformedTable(slot) => PenFailed(MalformedNode(slot))
                case Fetched(innerNode) =>
                  innerNode match
                    case FactDeny(_, denyItems) =>
                      val Span(denyFirst, denyEnd) = denyItems
                      var denyPen: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, separated, 5, "not (")
                      var denyIdx: U64 = denyFirst
                      while denyIdx < denyEnd && denyPen is Pen(_) do
                        @<lessOrEqual(denyIdx, denyEnd)>
                        val denyItem: U64 = lists(denyIdx)
                        val denySep: Stigi.Syntax.Print.Pen = if denyFirst < denyIdx then Stigi.Syntax.Print.putLiteral(tables, denyPen, 2, ", ") else denyPen
                        denyPen = if denyItem < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printPlainFact(tables, denyItem, denySep) else PenFailed(MalformedNode(denyIdx))
                        denyIdx = denyIdx + 1
                      Stigi.Syntax.Print.putLiteral(tables, denyPen, 1, ")")
                    case Absent(_) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactGroup(_, _) | Name(_, _) | TypeRef(_, _, _, _, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => Stigi.Syntax.Print.printPlainFact(tables, item, separated)
            else PenFailed(MalformedNode(index))
            index = index + 1
          Stigi.Syntax.Print.putLiteral(tables, current, 1, ">")
        case Name(_, _) | TypeRef(_, _, _, _, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
