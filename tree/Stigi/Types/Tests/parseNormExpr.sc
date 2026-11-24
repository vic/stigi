// prompt: verify Stigi.Types.lengthOf, sameLength, termOffsetOf, and TypeTable interning on parsed Checked-IR expressions.
def Stigi.Types.Tests.parseNormExpr {region: Stigi.Region, Stigi.Mut} (tables: Stigi.Syntax.Tables, scratch: Stigi.Elaborate.Scratch, context: Stigi.Types.LengthContext, srcCap: U64, src: Array[U8](srcCap)) <lessOrEqual(srcCap, Stigi.Limit.maxUnitBytes)>: Stigi.Types.LengthOutcome =
  val Filled(exprs) = Array.tryFill[Stigi.Core.Expr]{region}(32, NothingExpr(0)) else
    case FillFailed => LengthPoolFull
  val Filled(arms) = Array.tryFill[Stigi.Core.Arm]{region}(4, RestArm(0)) else
    case FillFailed => LengthPoolFull
  val Filled(childLists) = Array.tryFill[U64]{region}(8, 0) else
    case FillFailed => LengthPoolFull
  val Tables(synSource, _, synNodes, _, _, _) = tables
  Stigi.Syntax.Parse.parseUnit(tables, srcCap, src, srcCap) match
    case ParseRejected(_, _) | LexRejected(_) => LengthNotLinear
    case Parsed(rootSyn, _, _) =>
      synNodes(rootSyn) match
        case UnitRoot(_, _, _, declSyn) =>
          synNodes(declSyn) match
            case FunctionDecl(_, _, _, _, _, _, bodySyn) =>
              Stigi.Elaborate.lowerSyntaxNode(tables, bodySyn, CoreTables(32, exprs, 0, 0, 8, childLists, 0, 0, 4, arms, 0, 0), scratch) match
                case LowerOverflow | LowerInvalid => LengthNotLinear
                case Lowered(rootCore, loweredCore, _) =>
                  val CoreTables(_, _, _, exprCount, _, _, _, _, _, _, _, _) = loweredCore
                  var pos: U64 = 0
                  while pos < exprCount && pos < 32 do
                    @<lessOrEqual(pos, exprCount)>
                    exprs(pos) match
                      case LocalExpr(at, refFirst, typeSlot) =>
                        val leadByte: U64 = if refFirst < Stigi.Limit.maxUnitBytes then U64.ofU8(synSource(refFirst)) else 0
                        val slot: U64 = if leadByte == 110 then 0 else if leadByte == 107 then 1 else if leadByte == 97 then 2 else if leadByte == 98 then 3 else 4
                        exprs(pos) := LocalExpr(at, slot, typeSlot)
                      case GlobalExpr(_, _, _) | IntExpr(_, _, _, _) | TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | CallExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | ExactExpr(_, _, _, _, _) | CompareExpr(_, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | NegateExpr(_, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) => ()
                    pos = pos + 1
                  if rootCore < Stigi.Limit.maxUnitNodes then Stigi.Types.lengthOf(loweredCore, context, rootCore) else LengthNotLinear
            case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | UnitRoot(_, _, _, _) => LengthNotLinear
        case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) => LengthNotLinear
