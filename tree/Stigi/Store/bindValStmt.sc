// prompt: register a ValStmt pattern binder as KnownBinder when its initializer is linear, else ImmutableBinder.
def Stigi.Store.bindValStmt {Stigi.Mut} (synTables: Stigi.Syntax.Tables, image: Stigi.Names.Image, unitIdx: U64, state: Stigi.Store.LengthUnitState, patSyn: Stigi.Syntax.NodeAt, initSyn: Stigi.Syntax.NodeAt) <below(unitIdx, Stigi.Limit.maxUnits)>: Stigi.Store.LengthUnitState =
  val Tables(_, _, synNodes, _, _, _) = synTables
  state match
    case LengthUnitState(scratch, localCount, termCount, poolEnd, workTop, lanes) =>
      scratch match
        case LengthScratch(_, _, _, coreTables, elabScratch, _, pool, _, _, binders, globals, _, _, nodeStack, stepStack, formStack) =>
          synNodes(patSyn) match
            case PatName(_, Text(patFirst, patEnd)) =>
              var binderKind: Stigi.Types.BinderKind = ImmutableBinder
              var nextPool: U64 = poolEnd
              Stigi.Elaborate.lowerSyntaxNode(synTables, initSyn, coreTables, elabScratch) match
                case LowerOverflow | LowerInvalid => ()
                case Lowered(rootCore, loweredCore, _) =>
                  val CoreTables(_, _, _, exprCount, _, _, _, _, _, _, _, _) = loweredCore
                  if exprCount <= Stigi.Limit.maxUnitNodes && rootCore < Stigi.Limit.maxUnitNodes then
                    Stigi.Store.resolveLengthRefs(synTables, image, unitIdx, scratch, localCount, exprCount)
                    Stigi.Types.lengthOf(loweredCore, LengthSpace(1024, pool, poolEnd, 256, binders, Stigi.Limit.maxUnits, globals, nodeStack, stepStack, formStack), rootCore) match
                      case LengthValue(initForm, afterPool) =>
                        if afterPool <= 1024 then
                          binderKind = KnownBinder(initForm)
                          nextPool = afterPool
                        else ()
                      case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => ()
                  else ()
              Stigi.Store.registerLocalBinder(LengthUnitState(scratch, localCount, termCount, nextPool, workTop, lanes), patFirst, patEnd, binderKind)
            case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => state
