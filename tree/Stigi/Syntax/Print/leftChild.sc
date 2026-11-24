// prompt: inspect one slot along a postfix or infix left spine and decide if its child needs parentheses.
def Stigi.Syntax.Print.leftChild {Stigi.Mut} (tables: Stigi.Syntax.Tables, slot: Stigi.Syntax.NodeAt) <below(slot, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.LeftStep =
  Stigi.Syntax.readSlot(tables, slot) match
    case MalformedTable(_) => SpineLeaf
    case Fetched(found) =>
      found match
        case Binary(_, operator, left, _) => SpineNext(left, Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, left), Stigi.Syntax.Print.precedence(operator), false))
        case IsTest(_, subject, _) => SpineNext(subject, Stigi.Syntax.Print.subjectParens(Stigi.Syntax.Print.shapeOf(tables, subject)))
        case Call(_, callee, _, _, _, _) => SpineNext(callee, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, callee)))
        case IndexRead(_, target, _) => SpineNext(target, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, target)))
        case AssertStmt(_, subject, facts) => if Stigi.Syntax.Print.isAbsent(tables, subject) then BareAssert(facts) else SpineNext(subject, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, subject)))
        case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Unary(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => SpineLeaf
