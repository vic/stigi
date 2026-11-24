// prompt: the kind and pub flag of the unit whose parsed root node is given.
def Stigi.Names.shapeOfUnit(tables: Stigi.Syntax.Tables, root: Stigi.Syntax.NodeAt) <below(root, Stigi.Limit.maxUnitNodes)>: Stigi.Names.UnitShape =
  Stigi.Syntax.readSlot(tables, root) match
    case MalformedTable(_) => Unshaped
    case Fetched(rootNode) =>
      rootNode match
        case UnitRoot(_, _, _, decl) =>
          Stigi.Syntax.readSlot(tables, decl) match
            case MalformedTable(_) => Unshaped
            case Fetched(declNode) =>
              Stigi.Names.shapeOfDecl(declNode) match
                case Declared(kind, isPublic) => Shaped(kind, isPublic)
                case NotDeclaration => Unshaped
                case ValueDecl(isPublic, caps) =>
                  caps match
                    case Span(first, end) =>
                      Stigi.Names.roleOfCaps(tables, first, end, end - first + 1) match
                        case PlainValue => Shaped(ValKind, isPublic)
                        case TestValue => Shaped(TestKind, isPublic)
                        case NegTestValue => Shaped(NegTestKind, isPublic)
        case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) => Unshaped
