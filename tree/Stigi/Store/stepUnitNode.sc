// prompt: step one syntax node during unit length traversal, tracking binder scopes and recording length arguments.
def Stigi.Store.stepUnitNode {region: Stigi.Region, Stigi.Mut} (synTables: Stigi.Syntax.Tables, image: Stigi.Names.Image, unitIdx: U64, popped: Stigi.Store.LengthUnitState, node: Stigi.Syntax.NodeAt) <below(unitIdx, Stigi.Limit.maxUnits)>: Stigi.Store.StepUnitOutcome =
  val Tables(_, _, synNodes, _, _, _) = synTables
  popped match
    case LengthUnitState(_, localCount, _, _, _, _) =>
      synNodes(node) match
        case UnitRoot(_, _, _, decl) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(decl)))
        case FunctionDecl(_, _, _, _, head, clauses, body) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withBody: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(marked, VisitNode(body))
          val withClauses: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withBody, clauses, true)
          StepUnitDone(Stigi.Store.pushVisitWork(withClauses, VisitNode(head)))
        case DefHead(_, _, preGroups, _, params, postFacts, returns, exitFacts) =>
          val withExit: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, popped, exitFacts, true)
          val withRet: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(withExit, VisitNode(returns))
          val withPost: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withRet, postFacts, true)
          val withPre: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withPost, preGroups, true)
          val withParamNodes: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withPre, params, true)
          StepUnitDone(Stigi.Store.registerParamSpan(synTables, withParamNodes, params))
        case EnumDecl(_, _, _, _, cases) =>
          StepUnitDone(Stigi.Store.pushVisitSpan(synTables, popped, cases, true))
        case EnumCase(_, _, _, fields, facts) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withFacts: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(marked, VisitNode(facts))
          val withFields: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withFacts, fields, true)
          StepUnitDone(Stigi.Store.registerParamSpan(synTables, withFields, fields))
        case TypeDecl(_, _, _, _, valueParams, _, target, facts) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withFacts: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(marked, VisitNode(facts))
          val withTarget: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(withFacts, VisitNode(target))
          val withParams: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withTarget, valueParams, true)
          StepUnitDone(Stigi.Store.registerParamSpan(synTables, withParams, valueParams))
        case HandlerDecl(_, _, _, _, _, _, valueParams, target, vars, defs) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withDefs: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, marked, defs, true)
          val withVars: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withDefs, vars, true)
          val withTarget: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(withVars, VisitNode(target))
          val withParams: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, withTarget, valueParams, true)
          StepUnitDone(Stigi.Store.registerParamSpan(synTables, withParams, valueParams))
        case CapabilityDecl(_, _, _, _, _, _, _, members) =>
          StepUnitDone(Stigi.Store.pushVisitSpan(synTables, popped, members, true))
        case TraitDecl(_, _, _, _, members) =>
          StepUnitDone(Stigi.Store.pushVisitSpan(synTables, popped, members, true))
        case GivenDecl(_, _, _, _, _, target, defs) =>
          val withDefs: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, popped, defs, true)
          StepUnitDone(Stigi.Store.pushVisitWork(withDefs, VisitNode(target)))
        case ExternDef(_, _, _, _, head, body) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withBody: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(marked, VisitNode(body))
          StepUnitDone(Stigi.Store.pushVisitWork(withBody, VisitNode(head)))
        case ExternVal(_, _, _, _, declared) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(declared)))
        case ValDecl(_, _, _, _, declared, body) =>
          val withBody: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, VisitNode(body))
          StepUnitDone(Stigi.Store.pushVisitWork(withBody, VisitNode(declared)))
        case LocalDecl(_, decl) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(decl)))
        case Block(_, stmts) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          StepUnitDone(Stigi.Store.pushVisitSpan(synTables, marked, stmts, true))
        case ValStmt(_, pattern, declared, init, elseArms) =>
          val boundState: Stigi.Store.LengthUnitState = Stigi.Store.bindValStmt(synTables, image, unitIdx, popped, pattern, init)
          val withElse: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, boundState, elseArms, true)
          val withInit: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(withElse, VisitNode(init))
          StepUnitDone(Stigi.Store.pushVisitWork(withInit, VisitNode(declared)))
        case VarStmt(_, Text(varFirst, varEnd), _, declared, init) =>
          val regVar: Stigi.Store.LengthUnitState = Stigi.Store.registerLocalBinder(popped, varFirst, varEnd, MutableBinder)
          val withInit: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(regVar, VisitNode(init))
          StepUnitDone(Stigi.Store.pushVisitWork(withInit, VisitNode(declared)))
        case TypeRef(_, _, typeArgs, lengthArgs, _) =>
          Stigi.Store.processTypeRef{region}(synTables, image, unitIdx, popped, typeArgs, lengthArgs)
        case Call(_, callee, _, _, args, _) =>
          val withArgs: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, popped, args, true)
          StepUnitDone(Stigi.Store.pushVisitWork(withArgs, VisitNode(callee)))
        case Binary(_, _, left, right) =>
          val withRight: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, VisitNode(right))
          StepUnitDone(Stigi.Store.pushVisitWork(withRight, VisitNode(left)))
        case Unary(_, _, operand) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(operand)))
        case IsTest(_, subject, _) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(subject)))
        case IfElse(_, cond, whenTrue, whenFalse) =>
          val withFalse: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, VisitNode(whenFalse))
          val withTrue: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(withFalse, VisitNode(whenTrue))
          StepUnitDone(Stigi.Store.pushVisitWork(withTrue, VisitNode(cond)))
        case WhileLoop(_, cond, body) =>
          val withBody: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, VisitNode(body))
          StepUnitDone(Stigi.Store.pushVisitWork(withBody, VisitNode(cond)))
        case MatchOn(_, scrutinees, arms) =>
          val withArms: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitSpan(synTables, popped, arms, true)
          StepUnitDone(Stigi.Store.pushVisitSpan(synTables, withArms, scrutinees, true))
        case Arm(_, _, guard, body) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withBody: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(marked, VisitNode(body))
          StepUnitDone(Stigi.Store.pushVisitWork(withBody, VisitNode(guard)))
        case AssignStmt(_, _, valNode) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(valNode)))
        case IndexRead(_, target, idxNode) =>
          val withIdx: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, VisitNode(idxNode))
          StepUnitDone(Stigi.Store.pushVisitWork(withIdx, VisitNode(target)))
        case IndexWrite(_, target, idxNode, valNode) =>
          val withVal: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, VisitNode(valNode))
          val withIdx: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(withVal, VisitNode(idxNode))
          StepUnitDone(Stigi.Store.pushVisitWork(withIdx, VisitNode(target)))
        case HandleStmt(_, _, _, _, _, args, _, body) =>
          val marked: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(popped, RestoreBinders(localCount))
          val withBody: Stigi.Store.LengthUnitState = Stigi.Store.pushVisitWork(marked, VisitNode(body))
          StepUnitDone(Stigi.Store.pushVisitSpan(synTables, withBody, args, true))
        case AssertStmt(_, subject, _) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(subject)))
        case Param(_, _, declared) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(declared)))
        case CapParam(_, _, declared) =>
          StepUnitDone(Stigi.Store.pushVisitWork(popped, VisitNode(declared)))
        case Absent(_) | Name(_, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | DenyStmt(_, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) =>
          StepUnitDone(popped)
