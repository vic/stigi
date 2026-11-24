// prompt: print one simple non-recursive term atom of a length or fact expression, parenthesized when requested.
def Stigi.Syntax.Print.printTermAtom {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, parens: Bool, pen: Stigi.Syntax.Print.Pen) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  Stigi.Syntax.readSlot(tables, node) match
    case MalformedTable(slot) => PenFailed(MalformedNode(slot))
    case Fetched(found) =>
      val startPen: Stigi.Syntax.Print.Pen = if parens then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
      val bodyPen: Stigi.Syntax.Print.Pen = found match
        case Absent(_) => startPen
        case Name(_, text) => Stigi.Syntax.Print.putText(tables, startPen, text)
        case Ref(_, text) => Stigi.Syntax.Print.putText(tables, startPen, text)
        case Literal(_, kind, text) => if kind is UnitLiteral then Stigi.Syntax.Print.putLiteral(tables, startPen, 2, "()") else Stigi.Syntax.Print.putText(tables, startPen, text)
        case Unary(_, operator, operand) =>
          val innerParens: Bool = Stigi.Syntax.Print.operandParens(Stigi.Syntax.Print.shapeOf(tables, operand), operator is Negate)
          val operPen: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putUnary(tables, startPen, operator)
          val argStart: Stigi.Syntax.Print.Pen = if innerParens then Stigi.Syntax.Print.putLiteral(tables, operPen, 1, "(") else operPen
          val argMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.readSlot(tables, operand) match
            case MalformedTable(slot) => PenFailed(MalformedNode(slot))
            case Fetched(innerNode) =>
              innerNode match
                case Absent(_) => argStart
                case Name(_, text) => Stigi.Syntax.Print.putText(tables, argStart, text)
                case Ref(_, text) => Stigi.Syntax.Print.putText(tables, argStart, text)
                case Literal(_, kind, text) => if kind is UnitLiteral then Stigi.Syntax.Print.putLiteral(tables, argStart, 2, "()") else Stigi.Syntax.Print.putText(tables, argStart, text)
                case TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(operand))
          if innerParens then Stigi.Syntax.Print.putLiteral(tables, argMid, 1, ")") else argMid
        case TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
      if parens then Stigi.Syntax.Print.putLiteral(tables, bodyPen, 1, ")") else bodyPen
