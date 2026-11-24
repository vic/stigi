// prompt: fetch the pending action at a given step distance down an expression left spine.
def Stigi.Syntax.Print.findSpineStep {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, distance: U64) <lessOrEqual(distance, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.SpineAction =
  var walked: U64 = 0
  var cursor: U64 = node
  while walked < distance && cursor < Stigi.Limit.maxUnitNodes do
    @<lessOrEqual(walked, distance)>
    Stigi.Syntax.Print.leftChild(tables, cursor) match
      case SpineNext(nextChild, _) => cursor = nextChild
      case SpineLeaf | BareAssert(_) => cursor = Stigi.Limit.maxUnitNodes
    walked = walked + 1
  if !(cursor < Stigi.Limit.maxUnitNodes) then MissingSpine(node) else
    Stigi.Syntax.readSlot(tables, cursor) match
      case MalformedTable(slot) => MissingSpine(slot)
      case Fetched(found) =>
        found match
          case Binary(_, operator, left, right) => EmitBinary(operator, right, Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, left), Stigi.Syntax.Print.precedence(operator), false))
          case IsTest(_, subject, pattern) => EmitIs(pattern, Stigi.Syntax.Print.subjectParens(Stigi.Syntax.Print.shapeOf(tables, subject)))
          case Call(_, callee, typeArgs, capArgs, args, parens) => EmitCall(typeArgs, capArgs, args, parens, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, callee)))
          case IndexRead(_, target, index) => EmitIndex(index, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, target)))
          case AssertStmt(_, subject, facts) => EmitAssert(facts, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, subject)))
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Unary(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => MissingSpine(cursor)
