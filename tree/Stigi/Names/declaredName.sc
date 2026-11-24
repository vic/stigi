// prompt: the name text of a declaration node of the syntax table, or nothing when the node is no declaration.
def Stigi.Names.declaredName(tables: Stigi.Syntax.Tables, at: U64) <below(at, Stigi.Limit.maxUnitNodes)>: Stigi.Names.NameRead =
  Stigi.Syntax.readSlot(tables, at) match
    case MalformedTable(_) => Nameless
    case Fetched(node) =>
      node match
        case FunctionDecl(_, _, _, name, _, _, _) => NameFound(name)
        case ValDecl(_, _, name, _, _, _) => NameFound(name)
        case EnumDecl(_, _, name, _, _) => NameFound(name)
        case TypeDecl(_, _, name, _, _, _, _, _) => NameFound(name)
        case CapabilityDecl(_, _, name, _, _, _, _, _) => NameFound(name)
        case HandlerDecl(_, _, name, _, _, _, _, _, _, _) => NameFound(name)
        case TraitDecl(_, _, name, _, _) => NameFound(name)
        case GivenDecl(_, _, name, _, _, _, _) => NameFound(name)
        case ExternDef(_, _, _, name, _, _) => NameFound(name)
        case ExternVal(_, _, _, name, _) => NameFound(name)
        case RelationDecl(_, _, name, _) => NameFound(name)
        case RuleDecl(_, _, name, _, _, _) => NameFound(name)
        case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | EnumCase(_, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => Nameless
