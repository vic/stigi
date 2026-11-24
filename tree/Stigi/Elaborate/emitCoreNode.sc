// prompt: emit the postorder Checked-IR node of one visited syntax node and give back the tables and the scratch with the node recorded in the syntax-to-core map.
def Stigi.Elaborate.emitCoreNode {Stigi.Mut} (tables: Stigi.Syntax.Tables, synIdx: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch) <below(synIdx, Stigi.Limit.maxUnitNodes)>: Stigi.Elaborate.EmitResult =
  val Tables(source, _, synNodes, synLists, _, _) = tables
  scratch match
    case WorkScratch(capacity, work, top, mapped, patRes) =>
      core match
        case CoreTables(expressionCapacity, exprs, expressionMark, emitIdx, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm) =>
          synNodes(synIdx) match
            case Literal(_, litKind, litText) =>
              if synIdx < capacity && emitIdx < expressionCapacity then
                val nextCore: U64 = emitIdx + 1
                litText match
                  case Text(textFirst, textEnd) =>
                    val literalValid: Bool = litKind match
                      case IntLiteral =>
                        Stigi.Elaborate.parseSignedLiteral(Stigi.Limit.maxUnitBytes, source, textFirst, textEnd) match
                          case BadIntLiteral => false
                          case ParsedInt(numVal) =>
                            if numVal < 0 then false else
                              exprs(emitIdx) := IntExpr(emitIdx, U64.ofI64(numVal), 0, 0)
                              true
                      case CharLiteral =>
                        Stigi.Syntax.charValue(Stigi.Limit.maxUnitBytes, source, textFirst, textEnd) match
                          case CharMalformed => false
                          case CharDecoded(charVal) =>
                            exprs(emitIdx) := IntExpr(emitIdx, charVal, 0, 0)
                            true
                      case StringLiteral =>
                        exprs(emitIdx) := TextExpr(emitIdx, CoreSpan(textFirst, textEnd), 0)
                        true
                      case TrueLiteral =>
                        exprs(emitIdx) := TruthExpr(emitIdx, true)
                        true
                      case FalseLiteral =>
                        exprs(emitIdx) := TruthExpr(emitIdx, false)
                        true
                      case UnitLiteral =>
                        exprs(emitIdx) := NothingExpr(emitIdx)
                        true
                    if literalValid then
                      mapped(synIdx) := emitIdx
                      EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                    else EmitInvalid
              else EmitFull
            case Ref(_, refText) =>
              if synIdx < capacity && emitIdx < expressionCapacity then
                val nextCore: U64 = emitIdx + 1
                refText match
                  case Text(refFirst, _) =>
                    exprs(emitIdx) := LocalExpr(emitIdx, refFirst, 0)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
              else EmitFull
            case Binary(_, operator, leftSyn, rightSyn) =>
              if leftSyn < capacity && rightSyn < capacity then
                val leftCore: U64 = mapped(leftSyn)
                val rightCore: U64 = mapped(rightSyn)
                val operKind: Stigi.Syntax.OperKind = Stigi.Syntax.binOperKind(operator)
                operKind match
                  case OperPipe =>
                    val placed: Stigi.Elaborate.Placed = synNodes(rightSyn) match
                      case Call(_, calleeSyn, _, _, rhsArgs, _) => Stigi.Elaborate.placeCall(tables, rhsArgs, core, scratch, SelfGroupRef(calleeSyn), LeadingArgument(leftCore))
                      case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => Stigi.Elaborate.placeCall(tables, Span(0, 0), core, scratch, SelfGroupRef(rightSyn), LeadingArgument(leftCore))
                    placed match
                      case PlaceFull => EmitFull
                      case PlaceInvalid => EmitInvalid
                      case PlacedAt(placedCore, placedAt) =>
                        if synIdx < capacity then
                          mapped(synIdx) := placedAt
                          EmitDone(placedCore, WorkScratch(capacity, work, top, mapped, patRes))
                        else EmitFull
                  case OperPlus | OperMinus | OperTimes | OperDivide | OperRem | OperLess | OperAtMost | OperGreater | OperAtLeast | OperEquals | OperDiffers | OperAnd | OperOr =>
                    if leftCore < emitIdx && rightCore < emitIdx then
                      if synIdx < capacity && emitIdx < expressionCapacity then
                        val nextCore: U64 = emitIdx + 1
                        exprs(emitIdx) := Stigi.Elaborate.makeBinaryExpr(emitIdx, operKind, leftCore, rightCore)
                        mapped(synIdx) := emitIdx
                        EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                      else EmitFull
                    else EmitInvalid
              else EmitFull
            case Unary(_, _, operSyn) =>
              if operSyn < capacity then
                val operCore: U64 = mapped(operSyn)
                if operCore < emitIdx then
                  if synIdx < capacity && emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := NegateExpr(emitIdx, operCore)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
                else EmitInvalid
              else EmitFull
            case Call(_, calleeSyn, _, _, argsSpan, _) =>
              Stigi.Elaborate.placeCall(tables, argsSpan, core, scratch, SelfGroupRef(calleeSyn), WithoutLeading) match
                case PlaceFull => EmitFull
                case PlaceInvalid => EmitInvalid
                case PlacedAt(placedCore, placedAt) =>
                  if synIdx < capacity then
                    mapped(synIdx) := placedAt
                    EmitDone(placedCore, WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
            case IsTest(_, subjectSyn, patSyn) =>
              if subjectSyn < capacity then
                val subjCore: U64 = mapped(subjectSyn)
                val placed: Stigi.Elaborate.Placed = Stigi.Elaborate.lowerPattern(tables, patSyn, emitIdx, Stigi.Elaborate.markArms(core), scratch) match
                  case CoreFull => PlaceFull
                  case CoreInvalid => PlaceInvalid
                  case CoreDone(patterned) => Stigi.Elaborate.emitIsTest(patterned, subjCore)
                placed match
                  case PlaceFull => EmitFull
                  case PlaceInvalid => EmitInvalid
                  case PlacedAt(placedCore, placedAt) =>
                    if synIdx < capacity then
                      mapped(synIdx) := placedAt
                      EmitDone(placedCore, WorkScratch(capacity, work, top, mapped, patRes))
                    else EmitFull
              else EmitFull
            case MatchOn(_, scrutSpan, armsSpan) =>
              scrutSpan match
                case Span(scrutFirst, scrutEnd) =>
                  if scrutFirst < scrutEnd && scrutEnd - scrutFirst < 2 then
                    val scrutSyn: U64 = synLists(scrutFirst)
                    if scrutSyn < capacity then
                      val scrutCore: U64 = mapped(scrutSyn)
                      Stigi.Elaborate.lowerGuards(tables, armsSpan, scrutCore, core, scratch) match
                        case GuardsFull => EmitFull
                        case GuardsInvalid => EmitInvalid
                        case GuardsDone(guarded, guardedScratch) =>
                          Stigi.Elaborate.emitSelect(guarded, scrutCore) match
                            case PlaceFull => EmitFull
                            case PlaceInvalid => EmitInvalid
                            case PlacedAt(selected, selectAt) =>
                              guardedScratch match
                                case WorkScratch(capacity, work, top, mapped, patRes) =>
                                  if synIdx < capacity then
                                    mapped(synIdx) := selectAt
                                    EmitDone(selected, WorkScratch(capacity, work, top, mapped, patRes))
                                  else EmitFull
                    else EmitFull
                  else EmitInvalid
            case Arm(_, _, _, _) =>
              EmitDone(core, scratch)
            case IfElse(_, condSyn, thenSyn, elseSyn) =>
              if condSyn < capacity && thenSyn < capacity && elseSyn < capacity then
                val condCore: U64 = mapped(condSyn)
                val thenCore: U64 = mapped(thenSyn)
                val elseCore: U64 = mapped(elseSyn)
                if condCore < emitIdx && thenCore < emitIdx && elseCore < emitIdx then
                  if synIdx < capacity && emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := ChoiceExpr(emitIdx, condCore, thenCore, elseCore, 0)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
                else EmitInvalid
              else EmitFull
            case WhileLoop(_, condSyn, bodySyn) =>
              if condSyn < capacity && bodySyn < capacity then
                val condCore: U64 = mapped(condSyn)
                val bodyCore: U64 = mapped(bodySyn)
                if condCore < emitIdx && bodyCore < emitIdx then
                  if synIdx < capacity && emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := LoopExpr(emitIdx, condCore, CoreSpan(0, 0), Unmeasured, bodyCore)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
                else EmitInvalid
              else EmitFull
            case IndexRead(_, targetSyn, idxSyn) =>
              if targetSyn < capacity && idxSyn < capacity then
                val targetCore: U64 = mapped(targetSyn)
                val idxCore: U64 = mapped(idxSyn)
                if targetCore < emitIdx && idxCore < emitIdx then
                  if synIdx < capacity && emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := ElemReadExpr(emitIdx, targetCore, idxCore, 0)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
                else EmitInvalid
              else EmitFull
            case IndexWrite(_, targetSyn, idxSyn, valSyn) =>
              if targetSyn < capacity && idxSyn < capacity && valSyn < capacity then
                val targetCore: U64 = mapped(targetSyn)
                val idxCore: U64 = mapped(idxSyn)
                val valCore: U64 = mapped(valSyn)
                if targetCore < emitIdx && idxCore < emitIdx && valCore < emitIdx then
                  if synIdx < capacity && emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := ElemWriteExpr(emitIdx, targetCore, idxCore, valCore)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
                else EmitInvalid
              else EmitFull
            case Block(_, stmtsSpan) =>
              stmtsSpan match
                case Span(stmtFirst, stmtEnd) =>
                  if stmtFirst < stmtEnd then
                    val lastSyn: U64 = synLists(stmtEnd - 1)
                    if lastSyn < capacity then
                      if synIdx < capacity then
                        mapped(synIdx) := mapped(lastSyn)
                        EmitDone(core, WorkScratch(capacity, work, top, mapped, patRes))
                      else EmitFull
                    else EmitFull
                  else if synIdx < capacity && emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := NothingExpr(emitIdx)
                    mapped(synIdx) := emitIdx
                    EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
                  else EmitFull
            case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
              if synIdx < capacity && emitIdx < expressionCapacity then
                val nextCore: U64 = emitIdx + 1
                exprs(emitIdx) := NothingExpr(emitIdx)
                mapped(synIdx) := emitIdx
                EmitDone(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, curArm), WorkScratch(capacity, work, top, mapped, patRes))
              else EmitFull
