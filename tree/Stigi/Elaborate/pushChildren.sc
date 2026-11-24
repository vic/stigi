// prompt: push the leave item of one syntax node and enter items for its children onto the elaboration work stack.
def Stigi.Elaborate.pushChildren {Stigi.Mut} (tables: Stigi.Syntax.Tables, synIdx: U64, scratch: Stigi.Elaborate.Scratch) <below(synIdx, Stigi.Limit.maxUnitNodes)>: Stigi.Elaborate.PushResult =
  val Tables(_, _, synNodes, synLists, _, _) = tables
  Stigi.Elaborate.pushWork(scratch, Leave(synIdx)) match
    case PushFull => PushFull
    case PushDone(afterSelf) =>
      synNodes(synIdx) match
        case Binary(_, operator, leftSyn, rightSyn) =>
          Stigi.Syntax.binOperKind(operator) match
            case OperPipe =>
              val afterRhs: Stigi.Elaborate.PushResult = synNodes(rightSyn) match
                case Call(_, _, _, _, rhsArgs, _) =>
                  rhsArgs match
                    case Span(argFirst, argEnd) => Stigi.Elaborate.pushFrom(tables, rhsArgs, argFirst, afterSelf, argEnd - argFirst)
                case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
                  PushDone(afterSelf)
              afterRhs match
                case PushFull => PushFull
                case PushDone(afterArgs) => Stigi.Elaborate.pushWork(afterArgs, Enter(leftSyn))
            case OperPlus | OperMinus | OperTimes | OperDivide | OperRem | OperLess | OperAtMost | OperGreater | OperAtLeast | OperEquals | OperDiffers | OperAnd | OperOr =>
              Stigi.Elaborate.pushWork(afterSelf, Enter(rightSyn)) match
                case PushFull => PushFull
                case PushDone(afterRight) => Stigi.Elaborate.pushWork(afterRight, Enter(leftSyn))
        case Unary(_, _, operSyn) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(operSyn))
        case Call(_, _, _, _, argsSpan, _) =>
          argsSpan match
            case Span(argFirst, argEnd) => Stigi.Elaborate.pushFrom(tables, argsSpan, argFirst, afterSelf, argEnd - argFirst)
        case IsTest(_, subjectSyn, _) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(subjectSyn))
        case MatchOn(_, scrutSpan, armsSpan) =>
          armsSpan match
            case Span(armFirst, armEnd) =>
              Stigi.Elaborate.pushFrom(tables, armsSpan, armFirst, afterSelf, armEnd - armFirst) match
                case PushFull => PushFull
                case PushDone(afterArms) =>
                  scrutSpan match
                    case Span(scrutFirst, scrutEnd) => Stigi.Elaborate.pushFrom(tables, scrutSpan, scrutFirst, afterArms, scrutEnd - scrutFirst)
        case Arm(_, _, guardSyn, bodySyn) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(bodySyn)) match
            case PushFull => PushFull
            case PushDone(afterBody) =>
              if synNodes(guardSyn) is Absent(_) then PushDone(afterBody) else Stigi.Elaborate.pushWork(afterBody, Enter(guardSyn))
        case IfElse(_, condSyn, thenSyn, elseSyn) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(elseSyn)) match
            case PushFull => PushFull
            case PushDone(afterElse) =>
              Stigi.Elaborate.pushWork(afterElse, Enter(thenSyn)) match
                case PushFull => PushFull
                case PushDone(afterThen) => Stigi.Elaborate.pushWork(afterThen, Enter(condSyn))
        case WhileLoop(_, condSyn, bodySyn) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(bodySyn)) match
            case PushFull => PushFull
            case PushDone(afterBody) => Stigi.Elaborate.pushWork(afterBody, Enter(condSyn))
        case IndexRead(_, targetSyn, idxSyn) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(idxSyn)) match
            case PushFull => PushFull
            case PushDone(afterIndex) => Stigi.Elaborate.pushWork(afterIndex, Enter(targetSyn))
        case IndexWrite(_, targetSyn, idxSyn, valSyn) =>
          Stigi.Elaborate.pushWork(afterSelf, Enter(valSyn)) match
            case PushFull => PushFull
            case PushDone(afterValue) =>
              Stigi.Elaborate.pushWork(afterValue, Enter(idxSyn)) match
                case PushFull => PushFull
                case PushDone(afterIndex) => Stigi.Elaborate.pushWork(afterIndex, Enter(targetSyn))
        case Block(_, stmtsSpan) =>
          stmtsSpan match
            case Span(stmtFirst, stmtEnd) =>
              if stmtFirst < stmtEnd then
                val lastPos: U64 = stmtEnd - 1
                val lastSyn: Stigi.Syntax.NodeAt = synLists(lastPos)
                Stigi.Elaborate.pushWork(afterSelf, Enter(lastSyn))
              else
                PushDone(afterSelf)
        case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
          PushDone(afterSelf)
