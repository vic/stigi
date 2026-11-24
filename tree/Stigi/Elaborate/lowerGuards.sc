// prompt: lower the guarded arms of one match back to front into tail selects and choice nodes, then give back the tables, whose arm mark starts the full select arms, and the scratch.
def Stigi.Elaborate.lowerGuards {Stigi.Mut} (tables: Stigi.Syntax.Tables, armsSpan: Stigi.Syntax.Span, scrutCore: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch): Stigi.Elaborate.GuardResult =
  val Tables(_, _, synNodes, synLists, _, _) = tables
  scratch match
    case WorkScratch(capacity, work, top, mapped, patRes) =>
      armsSpan match
        case Span(first, endPos) =>
          var pos: U64 = endPos
          var current: Stigi.Core.Tables = Stigi.Elaborate.markArms(core)
          var state: Stigi.Elaborate.GuardState = Guarding
          while first < pos && state is Guarding do
            @<lessOrEqual(pos, endPos)>
            val prevPos: U64 = pos - 1
            val armSyn: Stigi.Syntax.NodeAt = synLists(prevPos)
            synNodes(armSyn) match
              case Arm(_, _, guardSyn, bodySyn) =>
                if bodySyn < capacity && guardSyn < capacity && armSyn < capacity then
                  val bodyCore: U64 = mapped(bodySyn)
                  if synNodes(guardSyn) is Absent(_) then
                    mapped(armSyn) := bodyCore
                  else if pos < endPos then
                    val guardCore: U64 = mapped(guardSyn)
                    val CoreTables(_, _, _, coreAt, _, _, _, _, _, _, _, _) = current
                    Stigi.Elaborate.lowerArms(tables, armsSpan, pos, coreAt, current, scratch, endPos - pos) match
                      case CoreFull => state = GuardsOverflowed
                      case CoreInvalid => state = GuardsRejected
                      case CoreDone(armed) =>
                        Stigi.Elaborate.emitGuardPair(armed, scrutCore, guardCore, bodyCore) match
                          case PlaceFull => state = GuardsOverflowed
                          case PlaceInvalid => state = GuardsRejected
                          case PlacedAt(paired, choiceAt) =>
                            mapped(armSyn) := choiceAt
                            current = Stigi.Elaborate.markArms(paired)
                  else
                    state = GuardsRejected
                else
                  state = GuardsRejected
              case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
                state = GuardsRejected
            pos = prevPos
          state match
            case GuardsOverflowed => GuardsFull
            case GuardsRejected => GuardsInvalid
            case Guarding =>
              val CoreTables(_, _, _, selectIdx, _, _, _, _, _, _, _, _) = current
              Stigi.Elaborate.lowerArms(tables, armsSpan, first, selectIdx, current, scratch, endPos - first) match
                case CoreFull => GuardsFull
                case CoreInvalid => GuardsInvalid
                case CoreDone(finished) => GuardsDone(finished, WorkScratch(capacity, work, top, mapped, patRes))
