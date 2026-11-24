// prompt: parse one unit text and record the members of its declaration for an owner.
def Stigi.Names.Tests.recordUnit {Stigi.Mut} (members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, textCap: U64, text: Array[U8](textCap), size: U64, owner: U64) <lessOrEqual(size, textCap), lessOrEqual(size, Stigi.Limit.maxUnitBytes), below(owner, Stigi.Limit.maxUnits)>: Stigi.Names.Recorded =
  Stigi.Syntax.Parse.parseUnit(tables, textCap, text, size) match
    case Parsed(root, _, _) =>
      Stigi.Syntax.readSlot(tables, root) match
        case MalformedTable(_) => MembersMalformed
        case Fetched(rootNode) =>
          rootNode match
            case UnitRoot(_, _, _, decl) =>
              Stigi.Syntax.readSlot(tables, decl) match
                case MalformedTable(_) => MembersMalformed
                case Fetched(declNode) => Stigi.Names.recordMembers(members, tables, owner, declNode)
            case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) => MembersMalformed
    case ParseRejected(_, _) | LexRejected(_) => MembersMalformed
