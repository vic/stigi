// prompt: print a parameter, capability parameter, or name node without node-depth recursion.
def Stigi.Syntax.Print.printParam {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  Stigi.Syntax.readSlot(tables, node) match
    case MalformedTable(slot) => PenFailed(MalformedNode(slot))
    case Fetched(found) =>
      found match
        case Absent(_) => pen
        case Name(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
        case Param(_, name, declared) => Stigi.Syntax.Print.printType(tables, declared, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, pen, name), 2, ": "), Stigi.Limit.maxParseDepth)
        case CapParam(_, name, declared) =>
          val named: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, name) then pen else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printTermAtom(tables, name, false, pen), 2, ": ")
          Stigi.Syntax.Print.printType(tables, declared, named, Stigi.Limit.maxParseDepth)
        case TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
