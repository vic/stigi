// prompt: print one non-recursive length term or capability name parsed by readOperand or readTerm.
def Stigi.Syntax.Print.printTerm {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  Stigi.Syntax.readSlot(tables, node) match
    case MalformedTable(slot) => PenFailed(MalformedNode(slot))
    case Fetched(found) =>
      found match
        case Binary(_, operator, left, right) =>
          val outer: U64 = Stigi.Syntax.Print.precedence(operator)
          val leftParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, left), outer, false)
          val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), outer, true)
          val lefted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermAtom(tables, left, leftParen, pen)
          Stigi.Syntax.Print.printTermAtom(tables, right, rightParen, Stigi.Syntax.Print.putBinary(tables, lefted, operator))
        case Absent(_) | Name(_, _) | Ref(_, _) | Literal(_, _, _) | Unary(_, _, _) => Stigi.Syntax.Print.printTermAtom(tables, node, false, pen)
        case TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Call(_, _, _, _, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
