// prompt: emit one postorder Checked-IR expression node for a visited syntax node.
def Stigi.Elaborate.emitCoreNode {Stigi.Mut} (tables: Stigi.Syntax.Tables, synIdx: U64, nodeCap: U64, exprs: Array[Stigi.Core.Expr](nodeCap), emitIdx: U64, listCap: U64, childLists: Array[U64](listCap), curList: U64, mapCap: U64, mapped: Array[U64](mapCap)) <below(synIdx, Stigi.Limit.maxUnitNodes), below(synIdx, mapCap), below(emitIdx, nodeCap), lessOrEqual(curList, listCap)>: Stigi.Elaborate.EmitResult =
  val Tables(source, _, synNodes, synLists, _, _) = tables
  val nextCore: U64 = emitIdx + 1
  synNodes(synIdx) match
    case Literal(_, litKind, litText) =>
      litText match
        case Text(textFirst, textEnd) =>
          litKind match
            case IntLiteral =>
              val numVal: U64 = Stigi.Elaborate.parseDecimal(Stigi.Limit.maxUnitBytes, source, textFirst, textEnd)
              exprs(emitIdx) := IntExpr(emitIdx, numVal, 0, 0)
            case CharLiteral =>
              val charVal: U64 = if textFirst < textEnd then U64.ofU8(source(textFirst)) else 0
              exprs(emitIdx) := IntExpr(emitIdx, charVal, 0, 0)
            case StringLiteral =>
              exprs(emitIdx) := TextExpr(emitIdx, CoreSpan(textFirst, textEnd), 0)
            case TrueLiteral =>
              exprs(emitIdx) := TruthExpr(emitIdx, true)
            case FalseLiteral =>
              exprs(emitIdx) := TruthExpr(emitIdx, false)
            case UnitLiteral =>
              exprs(emitIdx) := NothingExpr(emitIdx)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
    case Ref(_, refText) =>
      refText match
        case Text(refFirst, _) =>
          exprs(emitIdx) := LocalExpr(emitIdx, refFirst, 0)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
    case Binary(_, operator, leftSyn, rightSyn) =>
      if leftSyn >= mapCap || rightSyn >= mapCap then EmitFull else
        val leftCore: U64 = mapped(leftSyn)
        val rightCore: U64 = mapped(rightSyn)
        val operKind: Stigi.Syntax.OperKind = Stigi.Syntax.binOperKind(operator)
        operKind match
          case OperPipe =>
            if curList >= listCap then EmitFull else
              val afterLeft: U64 = curList + 1
              childLists(curList) := leftCore
              synNodes(rightSyn) match
                case Call(_, calleeSyn, _, _, rhsArgs, _) =>
                  Stigi.Elaborate.copyMappedSpan(tables, rhsArgs, listCap, childLists, afterLeft, mapCap, mapped) match
                    case EmitFull => EmitFull
                    case EmitInvalid => EmitInvalid
                    case EmitDone(_, endList) =>
                      if curList <= endList then
                        exprs(emitIdx) := CallExpr(emitIdx, SelfGroupRef(calleeSyn), CoreSpan(curList, endList), NormalCall, 0)
                        mapped(synIdx) := emitIdx
                        EmitDone(nextCore, endList)
                      else EmitFull
                case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _) =>
                  exprs(emitIdx) := CallExpr(emitIdx, SelfGroupRef(rightSyn), CoreSpan(curList, afterLeft), NormalCall, 0)
                  mapped(synIdx) := emitIdx
                  EmitDone(nextCore, afterLeft)
          case OperPlus | OperMinus | OperTimes | OperDivide | OperRem | OperLess | OperAtMost | OperGreater | OperAtLeast | OperEquals | OperDiffers | OperAnd | OperOr =>
            if leftCore < emitIdx && rightCore < emitIdx then
              exprs(emitIdx) := Stigi.Elaborate.makeBinaryExpr(emitIdx, operKind, leftCore, rightCore)
              mapped(synIdx) := emitIdx
              EmitDone(nextCore, curList)
            else EmitInvalid
    case Unary(_, _, operSyn) =>
      if operSyn >= mapCap then EmitFull else
        val operCore: U64 = mapped(operSyn)
        if operCore < emitIdx then
          exprs(emitIdx) := NegateExpr(emitIdx, operCore)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
        else EmitInvalid
    case Call(_, calleeSyn, _, _, argsSpan, _) =>
      Stigi.Elaborate.copyMappedSpan(tables, argsSpan, listCap, childLists, curList, mapCap, mapped) match
        case EmitFull => EmitFull
        case EmitInvalid => EmitInvalid
        case EmitDone(_, endList) =>
          if curList <= endList then
            exprs(emitIdx) := CallExpr(emitIdx, SelfGroupRef(calleeSyn), CoreSpan(curList, endList), NormalCall, 0)
            mapped(synIdx) := emitIdx
            EmitDone(nextCore, endList)
          else EmitFull
    case IfElse(_, condSyn, thenSyn, elseSyn) =>
      if condSyn < mapCap && thenSyn < mapCap && elseSyn < mapCap then
        val condCore: U64 = mapped(condSyn)
        val thenCore: U64 = mapped(thenSyn)
        val elseCore: U64 = mapped(elseSyn)
        if condCore < emitIdx && thenCore < emitIdx && elseCore < emitIdx then
          exprs(emitIdx) := ChoiceExpr(emitIdx, condCore, thenCore, elseCore, 0)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
        else EmitInvalid
      else EmitFull
    case WhileLoop(_, condSyn, bodySyn) =>
      if condSyn < mapCap && bodySyn < mapCap then
        val condCore: U64 = mapped(condSyn)
        val bodyCore: U64 = mapped(bodySyn)
        if condCore < emitIdx && bodyCore < emitIdx then
          exprs(emitIdx) := LoopExpr(emitIdx, condCore, CoreSpan(0, 0), Unmeasured, bodyCore)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
        else EmitInvalid
      else EmitFull
    case IndexRead(_, targetSyn, idxSyn) =>
      if targetSyn < mapCap && idxSyn < mapCap then
        val targetCore: U64 = mapped(targetSyn)
        val idxCore: U64 = mapped(idxSyn)
        if targetCore < emitIdx && idxCore < emitIdx then
          exprs(emitIdx) := ElemReadExpr(emitIdx, targetCore, idxCore, 0)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
        else EmitInvalid
      else EmitFull
    case IndexWrite(_, targetSyn, idxSyn, valSyn) =>
      if targetSyn < mapCap && idxSyn < mapCap && valSyn < mapCap then
        val targetCore: U64 = mapped(targetSyn)
        val idxCore: U64 = mapped(idxSyn)
        val valCore: U64 = mapped(valSyn)
        if targetCore < emitIdx && idxCore < emitIdx && valCore < emitIdx then
          exprs(emitIdx) := ElemWriteExpr(emitIdx, targetCore, idxCore, valCore)
          mapped(synIdx) := emitIdx
          EmitDone(nextCore, curList)
        else EmitInvalid
      else EmitFull
    case Block(_, stmtsSpan) =>
      stmtsSpan match
        case Span(stmtFirst, stmtEnd) =>
          if stmtFirst < stmtEnd then
            val lastSyn: U64 = synLists(stmtEnd - 1)
            if lastSyn < mapCap then
              mapped(synIdx) := mapped(lastSyn)
              EmitDone(emitIdx, curList)
            else EmitFull
          else
            exprs(emitIdx) := NothingExpr(emitIdx)
            mapped(synIdx) := emitIdx
            EmitDone(nextCore, curList)
    case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | IsTest(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _) =>
      exprs(emitIdx) := NothingExpr(emitIdx)
      mapped(synIdx) := emitIdx
      EmitDone(nextCore, curList)
