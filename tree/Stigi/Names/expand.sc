// prompt: expand one syntax node of a unit walk into the tasks and references it implies.
def Stigi.Names.expand {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, node: Stigi.Syntax.Node) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  node match
    case Ref(_, text) => Stigi.Names.referTo(walker, image, members, tables, from, text, true)
    case Block(_, statements) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, statements), NothingCounted)
    case ValStmt(_, pattern, declared, init, elseArms) =>
      Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Visit(pattern)), tables, elseArms), Visit(init)), Visit(declared)), NothingCounted)
    case PatName(_, text) =>
      val Text(first, end) = text
      if Stigi.Names.startsUpper(tables, first, end) then Stigi.Names.referTo(walker, image, members, tables, from, text, true) else Expanded(Stigi.Names.pushTask(walker, BindLocal(Local(text, WithoutCapability))), NothingCounted)
    case PatCtor(_, name, parts) =>
      val Expanded(referred, counted) = Stigi.Names.referTo(walker, image, members, tables, from, name, true)
      Expanded(Stigi.Names.pushSpan(referred, tables, parts), counted)
    case PatOr(_, alternatives) => Expanded(Stigi.Names.pushAlternatives(walker, tables, alternatives), NothingCounted)
    case MatchOn(_, scrutinees, arms) => Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(walker, tables, arms), tables, scrutinees), NothingCounted)
    case Arm(_, pattern, guard, body) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(body)), Visit(guard)), Visit(pattern)), NothingCounted)
    case IsTest(_, subject, pattern) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(pattern)), Visit(subject)), NothingCounted)
    case Call(_, callee, typeArgs, capArgs, args, _) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(walker, tables, args), tables, capArgs), tables, typeArgs), Visit(callee)), NothingCounted)
    case TypeRef(_, name, typeArgs, lengthArgs, capArg) =>
      val Expanded(referred, counted) = Stigi.Names.referTo(walker, image, members, tables, from, name, false)
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(referred, Visit(capArg)), tables, lengthArgs), tables, typeArgs), counted)
    case PatTuple(_, parts) => Expanded(Stigi.Names.pushSpan(walker, tables, parts), NothingCounted)
    case PatLiteral(_, _) => Expanded(walker, NothingCounted)
    case PatRange(_, low, high) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(high)), Visit(low)), NothingCounted)
    case Binary(_, _, left, right) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(right)), Visit(left)), NothingCounted)
    case Unary(_, _, operand) => Expanded(Stigi.Names.pushTask(walker, Visit(operand)), NothingCounted)
    case IndexRead(_, target, index) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(index)), Visit(target)), NothingCounted)
    case IndexWrite(_, target, index, value) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(value)), Visit(index)), Visit(target)), NothingCounted)
    case IfElse(_, condition, whenTrue, whenFalse) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(whenFalse)), Visit(whenTrue)), Visit(condition)), NothingCounted)
    case WhileLoop(_, condition, body) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(body)), Visit(condition)), NothingCounted)
    case VarStmt(_, name, invariant, declared, init) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(invariant)), BindLocal(Local(name, WithoutCapability))), Visit(init)), Visit(declared)), NothingCounted)
    case AssignStmt(_, name, value) =>
      val Expanded(referred, counted) = Stigi.Names.referTo(walker, image, members, tables, from, name, false)
      Expanded(Stigi.Names.pushTask(referred, Visit(value)), counted)
    case AssertStmt(_, subject, facts) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(facts)), Visit(subject)), NothingCounted)
    case DenyStmt(_, item) => Expanded(Stigi.Names.pushTask(walker, Visit(item)), NothingCounted)
    case Name(_, text) => Expanded(Stigi.Names.pushTask(walker, BindLocal(Local(text, WithoutCapability))), NothingCounted)
    case Param(_, name, declared) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(declared)), BindLocal(Local(name, WithoutCapability))), NothingCounted)
    case CapParam(_, name, declared) =>
      Stigi.Names.nameTextAt(tables, name) match
        case Nameless => Expanded(Stigi.Names.failWith(walker, TableMalformed), NothingCounted)
        case NameFound(text) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(declared)), BindLocal(Local(text, Stigi.Names.capabilityOf(image, members, tables, from, declared)))), NothingCounted)
    case CapGroup(_, caps) => Expanded(Stigi.Names.pushSpan(walker, tables, caps), NothingCounted)
    case DefHead(_, typeParams, preGroups, _, params, postFacts, returns, exitFacts) => Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushSpan(walker, tables, exitFacts), Visit(returns)), tables, postFacts), tables, params), tables, preGroups), tables, typeParams), NothingCounted)
    case FunctionDecl(_, _, _, _, head, clauses, body) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushTask(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(body)), tables, clauses), Visit(head)), NothingCounted)
    case FactAtom(_, _, evidence, name, args) =>
      val Expanded(referred, counted) = Stigi.Names.referTo(walker, image, members, tables, from, name, false)
      args match
        case Span(argFirst, argEnd) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushSpan(referred, tables, Stigi.Names.factAtomArgs(tables, name, argFirst, argEnd)), Visit(evidence)), counted)
    case FactCompare(_, _, evidence, left, _, right) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(right)), Visit(left)), Visit(evidence)), NothingCounted)
    case FactArith(_, _, evidence, target, left, _, right) =>
      val Expanded(referred, counted) = Stigi.Names.referTo(walker, image, members, tables, from, target, false)
      Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(referred, Visit(right)), Visit(left)), Visit(evidence)), counted)
    case FactDeny(_, items) => Expanded(Stigi.Names.pushSpan(walker, tables, items), NothingCounted)
    case FactGroup(_, items) => Expanded(Stigi.Names.pushSpan(walker, tables, items), NothingCounted)
    case HandleStmt(_, binder, invoked, typeArgs, capArgs, args, _, body) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      val Expanded(referred, counted) = Stigi.Names.referTo(walker, image, members, tables, from, invoked, false)
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(referred, Restore(localCount)), Visit(body)), Visit(binder)), tables, args), tables, capArgs), tables, typeArgs), counted)
    case LocalDecl(_, decl) =>
      Stigi.Names.declaredName(tables, decl) match
        case Nameless => Expanded(Stigi.Names.failWith(walker, TableMalformed), NothingCounted)
        case NameFound(text) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(decl)), BindLocal(Local(text, WithoutCapability))), NothingCounted)
    case ValDecl(_, _, _, caps, declared, body) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(body)), Visit(declared)), tables, caps), NothingCounted)
    case EnumDecl(_, _, _, typeParams, cases) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, cases), tables, typeParams), NothingCounted)
    case EnumCase(_, _, _, fields, facts) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(facts)), tables, fields), NothingCounted)
    case TypeDecl(_, _, _, typeParams, valueParams, capParams, target, facts) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(facts)), Visit(target)), tables, capParams), tables, valueParams), tables, typeParams), NothingCounted)
    case CapabilityDecl(_, _, _, typeParams, states, _, _, members) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, members), tables, states), tables, typeParams), NothingCounted)
    case HandlerDecl(_, _, _, typeParams, capParams, _, valueParams, target, vars, defs) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, defs), tables, vars), Visit(target)), tables, valueParams), tables, capParams), tables, typeParams), NothingCounted)
    case TraitDecl(_, _, _, typeParams, members) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, members), tables, typeParams), NothingCounted)
    case GivenDecl(_, _, _, typeParams, capParams, target, defs) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushSpan(Stigi.Names.pushTask(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, defs), Visit(target)), tables, capParams), tables, typeParams), NothingCounted)
    case ExternDef(_, _, _, _, head, body) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushTask(Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Restore(localCount)), Visit(body)), Visit(head)), NothingCounted)
    case ExternVal(_, _, _, _, declared) => Expanded(Stigi.Names.pushTask(walker, Visit(declared)), NothingCounted)
    case RelationDecl(_, _, _, columns) =>
      val Walker(_, _, _, localCount, _, _, _, _, _) = walker
      Expanded(Stigi.Names.pushSpan(Stigi.Names.pushTask(walker, Restore(localCount)), tables, columns), NothingCounted)
    case RuleDecl(_, _, _, head, body, _) => Expanded(Stigi.Names.pushTask(Stigi.Names.pushRuleSpan(walker, tables, body), VisitRule(head)), NothingCounted)
    case UnitRoot(_, _, _, decl) => Expanded(Stigi.Names.pushTask(walker, Visit(decl)), NothingCounted)
    case Absent(_) | Literal(_, _, _) | AliasLine(_, _, _) => Expanded(walker, NothingCounted)
