// prompt: pre-populate constant U64 global val unit bindings from the tree for resolving dotted length constants.
def Stigi.Store.collectValConstants {region: Stigi.Region, Stigi.Mut} (target: Stigi.Store.TypesTarget, scratch: Stigi.Store.LengthScratch, image: Stigi.Names.Image): Unit =
  val Image(_, entries, unitCount, _) = image
  scratch match
    case LengthScratch(_, _, synTables, coreTables, elabScratch, _, pool, _, _, binders, globals, _, _, nodeStack, stepStack, formStack) =>
      val Tables(_, _, synNodes, _, _, _) = synTables
      var unitIdx: U64 = 0
      while unitIdx < unitCount do
        @<lessOrEqual(unitIdx, unitCount)>
        val Entry(_, _, kind, _) = entries(unitIdx)
        if kind is ValKind then
          Stigi.Store.loadUnitForLength{region}(target, scratch, image, unitIdx) match
            case ParseRejected(_, _) | LexRejected(_) => ()
            case Parsed(rootSyn, _, _) =>
              synNodes(rootSyn) match
                case UnitRoot(_, _, _, declSyn) =>
                  synNodes(declSyn) match
                    case ValDecl(_, _, _, _, _, bodySyn) =>
                      Stigi.Elaborate.lowerSyntaxNode(synTables, bodySyn, coreTables, elabScratch) match
                        case LowerOverflow | LowerInvalid => ()
                        case Lowered(rootCore, loweredCore, _) =>
                          val CoreTables(_, _, _, exprCount, _, _, _, _, _, _, _, _) = loweredCore
                          if exprCount <= Stigi.Limit.maxUnitNodes && rootCore < Stigi.Limit.maxUnitNodes then
                            Stigi.Store.resolveLengthRefs(synTables, image, unitIdx, scratch, 0, exprCount)
                            Stigi.Types.lengthOf(loweredCore, LengthSpace(1024, pool, 0, 256, binders, Stigi.Limit.maxUnits, globals, nodeStack, stepStack, formStack), rootCore) match
                              case LengthValue(LinearForm(constWide, _, termCount), _) =>
                                if termCount == 0 then globals(unitIdx) := KnownBinder(LinearForm(constWide, 0, 0)) else ()
                              case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => ()
                          else ()
                    case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => ()
                case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) => ()
        else ()
        unitIdx = unitIdx + 1
