// prompt: the capability a binder of a declared type stands for: the unit of a capability or trait the type names, a builtin capability, else none.
def Stigi.Names.capabilityOf(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, declared: Stigi.Syntax.NodeAt) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.CapabilityBinding =
  Stigi.Syntax.readSlot(tables, declared) match
    case MalformedTable(_) => WithoutCapability
    case Fetched(node) =>
      node match
        case TypeRef(_, name, _, _, _) =>
          val Text(first, end) = name
          Stigi.Names.resolveText(image, members, tables, from, first, end, false) match
            case ToUnit(entry) => if entry < Stigi.Limit.maxUnits then Stigi.Names.capabilityUnit(image, entry) else WithoutCapability
            case ToBuiltin => BuiltinCapability
            case ToCase(_, _) | Unresolved | AmbiguousCase => WithoutCapability
        case Absent(_) | Name(_, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => WithoutCapability
