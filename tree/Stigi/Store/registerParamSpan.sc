// prompt: register every parameter in a DefHead params span as an ImmutableBinder for length resolution.
def Stigi.Store.registerParamSpan {Stigi.Mut} (synTables: Stigi.Syntax.Tables, state: Stigi.Store.LengthUnitState, params: Stigi.Syntax.Span): Stigi.Store.LengthUnitState =
  val Tables(_, _, synNodes, synLists, _, _) = synTables
  params match
    case Span(first, endPos) =>
      var current: Stigi.Store.LengthUnitState = state
      var pos: U64 = first
      while pos < endPos do
        @<lessOrEqual(first, pos), lessOrEqual(pos, endPos)>
        val paramSyn: Stigi.Syntax.NodeAt = synLists(pos)
        synNodes(paramSyn) match
          case Param(_, Text(nameFirst, nameEnd), _) =>
            current = Stigi.Store.registerLocalBinder(current, nameFirst, nameEnd, ImmutableBinder)
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => ()
        pos = pos + 1
      current
