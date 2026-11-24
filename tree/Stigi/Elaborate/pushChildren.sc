// prompt: push all child syntax nodes of one syntax node onto the elaboration work stack.
def Stigi.Elaborate.pushChildren {Stigi.Mut} (tables: Stigi.Syntax.Tables, synIdx: U64, workCap: U64, workNode: Array[U64](workCap), workPhase: Array[U8](workCap), topStart: U64) <below(synIdx, Stigi.Limit.maxUnitNodes), lessOrEqual(topStart, workCap)>: Stigi.Elaborate.PushResult =
  val Tables(_, _, synNodes, synLists, _, _) = tables
  Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topStart, synIdx, 1) match
    case PushFull => PushFull
    case PushDone(topAfterSelf) =>
      if topAfterSelf > workCap then PushFull else
        synNodes(synIdx) match
          case Binary(_, operator, leftSyn, rightSyn) =>
            Stigi.Syntax.binOperKind(operator) match
              case OperPipe =>
                val afterRhs: Stigi.Elaborate.PushResult = synNodes(rightSyn) match
                  case Call(_, _, _, _, rhsArgs, _) =>
                    Stigi.Elaborate.pushSpan(tables, rhsArgs, workCap, workNode, workPhase, topAfterSelf)
                  case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _) => PushDone(topAfterSelf)
                afterRhs match
                  case PushFull => PushFull
                  case PushDone(topMid) =>
                    if topMid <= workCap then Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topMid, leftSyn, 0) else PushFull
              case OperPlus | OperMinus | OperTimes | OperDivide | OperRem | OperLess | OperAtMost | OperGreater | OperAtLeast | OperEquals | OperDiffers | OperAnd | OperOr =>
                Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, rightSyn, 0) match
                  case PushFull => PushFull
                  case PushDone(topMid) =>
                    if topMid <= workCap then Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topMid, leftSyn, 0) else PushFull
          case Unary(_, _, operSyn) =>
            Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, operSyn, 0)
          case Call(_, _, _, _, argsSpan, _) =>
            Stigi.Elaborate.pushSpan(tables, argsSpan, workCap, workNode, workPhase, topAfterSelf)
          case IfElse(_, condSyn, thenSyn, elseSyn) =>
            Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, elseSyn, 0) match
              case PushFull => PushFull
              case PushDone(topOne) =>
                if topOne <= workCap then
                  Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topOne, thenSyn, 0) match
                    case PushFull => PushFull
                    case PushDone(topTwo) =>
                      if topTwo <= workCap then Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topTwo, condSyn, 0) else PushFull
                else PushFull
          case WhileLoop(_, condSyn, bodySyn) =>
            Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, bodySyn, 0) match
              case PushFull => PushFull
              case PushDone(topOne) =>
                if topOne <= workCap then Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topOne, condSyn, 0) else PushFull
          case IndexRead(_, targetSyn, idxSyn) =>
            Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, idxSyn, 0) match
              case PushFull => PushFull
              case PushDone(topOne) =>
                if topOne <= workCap then Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topOne, targetSyn, 0) else PushFull
          case IndexWrite(_, targetSyn, idxSyn, valSyn) =>
            Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, valSyn, 0) match
              case PushFull => PushFull
              case PushDone(topOne) =>
                if topOne <= workCap then
                  Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topOne, idxSyn, 0) match
                    case PushFull => PushFull
                    case PushDone(topTwo) =>
                      if topTwo <= workCap then Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topTwo, targetSyn, 0) else PushFull
                else PushFull
          case Block(_, stmtsSpan) =>
            stmtsSpan match
              case Span(stmtFirst, stmtEnd) =>
                if stmtFirst < stmtEnd then
                  val lastPos: U64 = stmtEnd - 1
                  Stigi.Elaborate.pushWork(workCap, workNode, workPhase, topAfterSelf, synLists(lastPos), 0)
                else
                  PushDone(topAfterSelf)
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | IsTest(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _) =>
            PushDone(topAfterSelf)
