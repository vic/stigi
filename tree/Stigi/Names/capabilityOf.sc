// prompt: the capability a binder of a declared type stands for: the unit of a capability or trait the type names, a builtin capability, else none; a declared node that the table cannot read or that is not a type reference is a malformed read.
def Stigi.Names.capabilityOf(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, declared: Stigi.Syntax.NodeAt) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.CapabilityRead =
  Stigi.Syntax.readSlot(tables, declared) match
    case MalformedTable(_) => CapabilityMalformed
    case Fetched(node) =>
      node match
        case TypeRef(_, name, _, _, _) =>
          Stigi.Names.resolveText(NameScope(image, members), tables, from, name, false) match
            case ToUnit(entry) => Stigi.Names.capabilityUnit(image, entry)
            case ToBuiltin => CapabilityFound(BuiltinCapability)
            case ToCase(_, _) | Unresolved | AmbiguousCase => CapabilityFound(WithoutCapability)
        case Absent(_) | Name(_, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => CapabilityMalformed
