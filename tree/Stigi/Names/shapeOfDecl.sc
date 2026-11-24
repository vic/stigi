// prompt: the shape a declaration node gives its unit.
def Stigi.Names.shapeOfDecl(node: Stigi.Syntax.Node): Stigi.Names.DeclShape =
  node match
    case FunctionDecl(_, isPublic, _, _, _, _, _) => Declared(DefKind, isPublic)
    case ValDecl(_, isPublic, _, caps, _, _) => ValueDecl(isPublic, caps)
    case EnumDecl(_, isPublic, _, _, _) => Declared(EnumKind, isPublic)
    case TypeDecl(_, isPublic, _, _, _, _, _, _) => Declared(TypeKind, isPublic)
    case CapabilityDecl(_, isPublic, _, _, _, _, _, _) => Declared(CapabilityKind, isPublic)
    case HandlerDecl(_, isPublic, _, _, _, _, _, _, _, _) => Declared(HandlerKind, isPublic)
    case TraitDecl(_, isPublic, _, _, _) => Declared(TraitKind, isPublic)
    case GivenDecl(_, isPublic, _, _, _, _, _) => Declared(GivenKind, isPublic)
    case ExternDef(_, isPublic, _, _, _, _) => Declared(ExternKind, isPublic)
    case ExternVal(_, isPublic, _, _, _) => Declared(ExternKind, isPublic)
    case RelationDecl(_, isPublic, _, _) => Declared(RelationKind, isPublic)
    case RuleDecl(_, isPublic, _, _, _, _) => Declared(RuleKind, isPublic)
    case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | EnumCase(_, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => NotDeclaration
