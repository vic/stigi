// prompt: lower the arms of one match span from one position into one-level decision arms whose bodies are already emitted below the select, the decreasing count bounding the recursion.
def Stigi.Elaborate.lowerArms {Stigi.Mut} (tables: Stigi.Syntax.Tables, armsSpan: Stigi.Syntax.Span, position: U64, selectIdx: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch, remaining: U64) <decreases(remaining)>: Stigi.Elaborate.CoreResult =
  armsSpan match
    case Span(_, endPos) =>
      if remaining < 1 || endPos <= position then CoreDone(core) else
        @<below(position, Stigi.Limit.maxUnitNodes)>
        val Tables(_, _, synNodes, synLists, _, _) = tables
        val armSyn: Stigi.Syntax.NodeAt = synLists(position)
        synNodes(armSyn) match
          case Arm(_, patSyn, _, _) =>
            scratch match
              case WorkScratch(capacity, _, _, mapped, _) =>
                if armSyn < capacity then
                  val bodyAt: U64 = mapped(armSyn)
                  if bodyAt < selectIdx then
                    Stigi.Elaborate.lowerPattern(tables, patSyn, bodyAt, core, scratch) match
                      case CoreFull => CoreFull
                      case CoreInvalid => CoreInvalid
                      case CoreDone(next) => Stigi.Elaborate.lowerArms(tables, armsSpan, position + 1, selectIdx, next, scratch, remaining - 1)
                  else CoreInvalid
                else CoreInvalid
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => CoreInvalid
