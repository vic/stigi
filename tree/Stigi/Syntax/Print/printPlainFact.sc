// prompt: print a plain fact node without expression-depth recursion.
def Stigi.Syntax.Print.printPlainFact {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  Stigi.Syntax.readSlot(tables, node) match
    case MalformedTable(slot) => PenFailed(MalformedNode(slot))
    case Fetched(found) =>
      found match
        case Absent(_) => pen
        case FactAtom(_, negated, evidence, name, args) =>
          val led: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putFactLead(tables, pen, negated, evidence)
          Stigi.Syntax.Print.printTermList(tables, args, InDelimiters(InParens), Stigi.Syntax.Print.putText(tables, led, name))
        case FactCompare(_, negated, evidence, left, operator, right) =>
          val led: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putFactLead(tables, pen, negated, evidence)
          Stigi.Syntax.Print.printTerm(tables, right, Stigi.Syntax.Print.putBinary(tables, Stigi.Syntax.Print.printTerm(tables, left, led), operator))
        case FactArith(_, negated, evidence, target, left, operator, right) =>
          val assigned: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putFactLead(tables, pen, negated, evidence), target), 3, " = ")
          Stigi.Syntax.Print.printTerm(tables, right, Stigi.Syntax.Print.putBinary(tables, Stigi.Syntax.Print.printTerm(tables, left, assigned), operator))
        case Name(_, _) | TypeRef(_, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
