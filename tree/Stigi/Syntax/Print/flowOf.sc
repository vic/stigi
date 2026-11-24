// prompt: the line flow of one syntax node.
def Stigi.Syntax.Print.flowOf(node: Stigi.Syntax.Node) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Flow =
  case Block(_, statements) => BlockFlow(statements)
  case MatchOn(_, _, _) | WhileLoop(_, _, _) => AlwaysMultiline
  case IfElse(_, _, whenTrue, whenFalse) => BranchFlow(whenTrue, whenFalse)
  case HandleStmt(_, _, _, _, _, _, _, body) => ChildFlow(body)
  case ValStmt(_, _, _, init, _) => ChildFlow(init)
  case VarStmt(_, _, _, _, init) => ChildFlow(init)
  case AssignStmt(_, _, value) => ChildFlow(value)
  case IndexWrite(_, _, _, value) => ChildFlow(value)
  case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | Arm(_, _, _, _) | IndexRead(_, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _) => SingleLine
