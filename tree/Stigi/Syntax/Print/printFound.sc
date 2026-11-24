// prompt: print one syntax node in the canonical layout of the seed printer.
def Stigi.Syntax.Print.printFound {Stigi.Mut} (tables: Stigi.Syntax.Tables, found: Stigi.Syntax.Node, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Limit.maxUnitBytes <= indent then PenOverflow else
    val inner: U64 = depth - 1
    found match
      case Absent(_) => pen
      case Name(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
      case TypeRef(_, name, typeArgs, lengthArgs, capArg) =>
        val sized: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.wrapped(tables, lengthArgs, InParens, Stigi.Syntax.Print.wrapped(tables, typeArgs, InBrackets, Stigi.Syntax.Print.putText(tables, pen, name), indent, inner), indent, inner)
        if Stigi.Syntax.Print.isAbsent(tables, capArg) then sized else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, capArg, Stigi.Syntax.Print.putLiteral(tables, sized, 1, "{"), indent, inner), 1, "}")
      case FactAtom(_, negated, evidence, name, args) => Stigi.Syntax.Print.enclosed(tables, args, InParens, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.factLead(tables, negated, evidence, pen, indent, inner), name), indent, inner)
      case FactCompare(_, negated, evidence, left, operator, right) =>
        val lefted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printParened(tables, left, Stigi.Syntax.Print.termParens(Stigi.Syntax.Print.shapeOf(tables, left)), Stigi.Syntax.Print.factLead(tables, negated, evidence, pen, indent, inner), indent, inner)
        Stigi.Syntax.Print.printParened(tables, right, Stigi.Syntax.Print.termParens(Stigi.Syntax.Print.shapeOf(tables, right)), Stigi.Syntax.Print.putBinary(tables, lefted, operator), indent, inner)
      case FactArith(_, negated, evidence, target, left, operator, right) =>
        val assigned: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.factLead(tables, negated, evidence, pen, indent, inner), target), 3, " = ")
        val lefted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printParened(tables, left, Stigi.Syntax.Print.termParens(Stigi.Syntax.Print.shapeOf(tables, left)), assigned, indent, inner)
        Stigi.Syntax.Print.printParened(tables, right, Stigi.Syntax.Print.termParens(Stigi.Syntax.Print.shapeOf(tables, right)), Stigi.Syntax.Print.putBinary(tables, lefted, operator), indent, inner)
      case FactDeny(_, items) => Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printList(tables, items, JoinedByComma, Stigi.Syntax.Print.putLiteral(tables, pen, 5, "not ("), indent, inner), 1, ")")
      case FactGroup(_, items) => Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printList(tables, items, JoinedByComma, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "<"), indent, inner), 1, ">")
      case Param(_, name, declared) => Stigi.Syntax.Print.printNode(tables, declared, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, pen, name), 2, ": "), indent, inner)
      case CapParam(_, name, declared) =>
        val named: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, name) then pen else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, name, pen, indent, inner), 2, ": ")
        Stigi.Syntax.Print.printNode(tables, declared, named, indent, inner)
      case CapGroup(_, caps) => Stigi.Syntax.Print.enclosed(tables, caps, InBraces, pen, indent, inner)
      case PatName(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
      case PatLiteral(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
      case PatCtor(_, name, parts) => Stigi.Syntax.Print.enclosed(tables, parts, InParens, Stigi.Syntax.Print.putText(tables, pen, name), indent, inner)
      case PatTuple(_, parts) => Stigi.Syntax.Print.enclosed(tables, parts, InParens, pen, indent, inner)
      case PatOr(_, alternatives) => Stigi.Syntax.Print.printList(tables, alternatives, JoinedByBar, pen, indent, inner)
      case PatRange(_, low, high) => Stigi.Syntax.Print.printNode(tables, high, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, low, pen, indent, inner), 2, ".."), indent, inner)
      case Literal(_, kind, text) => if kind is UnitLiteral then Stigi.Syntax.Print.putLiteral(tables, pen, 2, "()") else Stigi.Syntax.Print.putText(tables, pen, text)
      case Ref(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
      case Call(_, callee, typeArgs, capArgs, args, parens) =>
        val called: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printParened(tables, callee, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, callee)), pen, indent, inner)
        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.wrapped(tables, capArgs, InBraces, Stigi.Syntax.Print.wrapped(tables, typeArgs, InBrackets, called, indent, inner), indent, inner)
        if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.enclosed(tables, args, InParens, capped, indent, inner) else capped
      case Binary(_, operator, left, right) =>
        val outer: U64 = Stigi.Syntax.Print.precedence(operator)
        val lefted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printParened(tables, left, Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, left), outer, false), pen, indent, inner)
        Stigi.Syntax.Print.printParened(tables, right, Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), outer, true), Stigi.Syntax.Print.putBinary(tables, lefted, operator), indent, inner)
      case Unary(_, operator, operand) => Stigi.Syntax.Print.printParened(tables, operand, Stigi.Syntax.Print.operandParens(Stigi.Syntax.Print.shapeOf(tables, operand), operator is Negate), Stigi.Syntax.Print.putUnary(tables, pen, operator), indent, inner)
      case IsTest(_, subject, pattern) => Stigi.Syntax.Print.printNode(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printParened(tables, subject, Stigi.Syntax.Print.subjectParens(Stigi.Syntax.Print.shapeOf(tables, subject)), pen, indent, inner), 4, " is "), indent, inner)
      case IfElse(_, condition, whenTrue, whenFalse) =>
        val tested: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, condition, Stigi.Syntax.Print.putLiteral(tables, pen, 3, "if "), indent, inner), 5, " then")
        val thenDone: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printBody(tables, whenTrue, tested, indent, inner)
        val elsed: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isMultiline(tables, whenTrue, Stigi.Limit.maxParseDepth) then Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putNewline(tables, thenDone, indent), 4, "else") else Stigi.Syntax.Print.putLiteral(tables, thenDone, 5, " else")
        Stigi.Syntax.Print.printBody(tables, whenFalse, elsed, indent, inner)
      case WhileLoop(_, condition, body) => Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, condition, Stigi.Syntax.Print.putLiteral(tables, pen, 6, "while "), indent, inner), 3, " do"), indent, inner)
      case MatchOn(_, scrutinees, arms) => Stigi.Syntax.Print.printList(tables, arms, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printScrutinees(tables, scrutinees, pen, indent, inner), 6, " match"), indent + 2, inner)
      case Arm(_, pattern, guard, body) =>
        val guarded: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printOptional(tables, guard, 4, " if ", Stigi.Syntax.Print.printNode(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, pen, 5, "case "), indent, inner), indent, inner)
        Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, guarded, 3, " =>"), indent, inner)
      case ValStmt(_, pattern, declared, init, elseArms) =>
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printOptional(tables, declared, 2, ": ", Stigi.Syntax.Print.printNode(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, pen, 4, "val "), indent, inner), indent, inner)
        val valued: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printBody(tables, init, Stigi.Syntax.Print.putLiteral(tables, typed, 2, " ="), indent, inner)
        if Stigi.Syntax.Print.hasItems(elseArms) then Stigi.Syntax.Print.printList(tables, elseArms, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, valued, 5, " else"), indent + 2, inner) else valued
      case VarStmt(_, name, invariant, declared, init) =>
        val held: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printOptional(tables, invariant, 1, "@", Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 4, "var "), name), indent, inner)
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printOptional(tables, declared, 2, ": ", held, indent, inner)
        Stigi.Syntax.Print.printBody(tables, init, Stigi.Syntax.Print.putLiteral(tables, typed, 2, " ="), indent, inner)
      case AssignStmt(_, name, value) => Stigi.Syntax.Print.printBody(tables, value, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, pen, name), 2, " ="), indent, inner)
      case IndexRead(_, target, index) =>
        val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printParened(tables, target, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, target)), pen, indent, inner)
        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, "("), indent, inner), 1, ")")
      case IndexWrite(_, target, index, value) =>
        val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printParened(tables, target, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, target)), pen, indent, inner)
        val indexed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, "("), indent, inner), 4, ") :=")
        Stigi.Syntax.Print.printBody(tables, value, indexed, indent, inner)
      case HandleStmt(_, binder, invoked, typeArgs, capArgs, args, parens, body) =>
        val started: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, pen, 7, "handle ")
        val bound: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, binder) then started else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, binder, started, indent, inner), 3, " = ")
        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.wrapped(tables, capArgs, InBraces, Stigi.Syntax.Print.wrapped(tables, typeArgs, InBrackets, Stigi.Syntax.Print.putText(tables, bound, invoked), indent, inner), indent, inner)
        val called: Stigi.Syntax.Print.Pen = if parens then Stigi.Syntax.Print.enclosed(tables, args, InParens, capped, indent, inner) else capped
        if Stigi.Syntax.Print.isAbsent(tables, body) then called else Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, called, 1, ":"), indent, inner)
      case AssertStmt(_, subject, facts) =>
        val subjected: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, subject) then pen else Stigi.Syntax.Print.printParened(tables, subject, Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, subject)), pen, indent, inner)
        Stigi.Syntax.Print.printNode(tables, facts, Stigi.Syntax.Print.putLiteral(tables, subjected, 1, "@"), indent, inner)
      case DenyStmt(_, item) => Stigi.Syntax.Print.printNode(tables, item, pen, indent, inner)
      case Block(_, statements) => Stigi.Syntax.Print.printList(tables, statements, StackedLines, pen, indent, inner)
      case LocalDecl(_, decl) => Stigi.Syntax.Print.printNode(tables, decl, pen, indent, inner)
      case DefHead(_, typeParams, preGroups, valueGroup, params, postFacts, result, exitFacts) =>
        val grouped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printList(tables, preGroups, EachAfterSpace, Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, pen, indent, inner), indent, inner)
        val spaced: Stigi.Syntax.Print.Pen = if valueGroup && (Stigi.Syntax.Print.hasItems(typeParams) || Stigi.Syntax.Print.hasItems(preGroups)) then Stigi.Syntax.Print.putLiteral(tables, grouped, 1, " ") else grouped
        val valued: Stigi.Syntax.Print.Pen = if valueGroup then Stigi.Syntax.Print.enclosed(tables, params, InParens, spaced, indent, inner) else spaced
        val returned: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printOptional(tables, result, 1, " ", Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printList(tables, postFacts, EachAfterSpace, valued, indent, inner), 1, ":"), indent, inner)
        Stigi.Syntax.Print.printList(tables, exitFacts, EachAfterSpace, returned, indent, inner)
      case FunctionDecl(_, isPublic, flavor, name, head, clauses, body) =>
        val headed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, head, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putFlavor(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), flavor), name), indent, inner)
        if Stigi.Syntax.Print.hasItems(clauses) then Stigi.Syntax.Print.printList(tables, clauses, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, headed, 2, " ="), indent + 2, inner) else if Stigi.Syntax.Print.isAbsent(tables, body) then headed else Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, headed, 2, " ="), indent, inner)
      case ValDecl(_, isPublic, name, caps, declared, body) =>
        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, caps, InBraces, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 4, "val "), name), indent, inner)
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, declared, Stigi.Syntax.Print.putLiteral(tables, capped, 2, ": "), indent, inner)
        Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, typed, 2, " ="), indent, inner)
      case EnumDecl(_, isPublic, name, typeParams, cases) =>
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 5, "enum "), name), indent, inner)
        Stigi.Syntax.Print.printList(tables, cases, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, typed, 1, ":"), indent + 2, inner)
      case EnumCase(_, name, parens, fields, facts) =>
        val named: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 5, "case "), name)
        val fielded: Stigi.Syntax.Print.Pen = if parens then Stigi.Syntax.Print.enclosed(tables, fields, InParens, named, indent, inner) else named
        Stigi.Syntax.Print.printOptional(tables, facts, 1, " ", fielded, indent, inner)
      case TypeDecl(_, isPublic, name, typeParams, valueParams, capParams, target, facts) =>
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 5, "type "), name), indent, inner)
        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, capParams, InBraces, Stigi.Syntax.Print.spacedWrapped(tables, valueParams, InParens, typed, indent, inner), indent, inner)
        Stigi.Syntax.Print.printOptional(tables, facts, 1, " ", Stigi.Syntax.Print.printNode(tables, target, Stigi.Syntax.Print.putLiteral(tables, capped, 3, " = "), indent, inner), indent, inner)
      case CapabilityDecl(_, isPublic, name, typeParams, states, startState, stops, members) =>
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 11, "capability "), name), indent, inner)
        val stated: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printStates(tables, states, startState, stops, Stigi.Syntax.Print.putLiteral(tables, typed, 1, ":"), indent + 2, inner)
        Stigi.Syntax.Print.printList(tables, members, EachOnLine, stated, indent + 2, inner)
      case HandlerDecl(_, isPublic, name, typeParams, capParams, valueGroup, valueParams, target, vars, defs) =>
        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, capParams, InBraces, Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 8, "handler "), name), indent, inner), indent, inner)
        val spaced: Stigi.Syntax.Print.Pen = if valueGroup && (Stigi.Syntax.Print.hasItems(typeParams) || Stigi.Syntax.Print.hasItems(capParams)) then Stigi.Syntax.Print.putLiteral(tables, capped, 1, " ") else capped
        val valued: Stigi.Syntax.Print.Pen = if valueGroup then Stigi.Syntax.Print.enclosed(tables, valueParams, InParens, spaced, indent, inner) else spaced
        val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, target, Stigi.Syntax.Print.putLiteral(tables, valued, 5, " for "), indent, inner)
        Stigi.Syntax.Print.printList(tables, defs, EachOnLine, Stigi.Syntax.Print.printList(tables, vars, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, ":"), indent + 2, inner), indent + 2, inner)
      case TraitDecl(_, isPublic, name, typeParams, members) =>
        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 6, "trait "), name), indent, inner)
        Stigi.Syntax.Print.printList(tables, members, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, typed, 1, ":"), indent + 2, inner)
      case GivenDecl(_, isPublic, name, typeParams, capParams, target, defs) =>
        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.spacedWrapped(tables, capParams, InBraces, Stigi.Syntax.Print.spacedWrapped(tables, typeParams, InBrackets, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 6, "given "), name), indent, inner), indent, inner)
        val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, target, Stigi.Syntax.Print.putLiteral(tables, capped, 2, ": "), indent, inner)
        Stigi.Syntax.Print.printList(tables, defs, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, ":"), indent + 2, inner)
      case ExternDef(_, isPublic, abi, name, head, body) =>
        val named: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 7, "extern "), abi), 5, " def "), name)
        val headed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, head, named, indent, inner)
        if Stigi.Syntax.Print.isAbsent(tables, body) then headed else Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, headed, 2, " ="), indent, inner)
      case ExternVal(_, isPublic, abi, name, declared) =>
        val named: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 7, "extern "), abi), 5, " val "), name)
        Stigi.Syntax.Print.printNode(tables, declared, Stigi.Syntax.Print.putLiteral(tables, named, 2, ": "), indent, inner)
      case RelationDecl(_, isPublic, name, columns) => Stigi.Syntax.Print.enclosed(tables, columns, InParens, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 9, "relation "), name), indent, inner)
      case RuleDecl(_, isPublic, name, head, body, multiline) =>
        val headed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, head, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 9, "rule def "), name), 2, ": "), indent, inner)
        val opened: Stigi.Syntax.Print.Pen = if multiline then Stigi.Syntax.Print.putSpaces(tables, Stigi.Syntax.Print.putLiteral(tables, headed, 3, " =\n"), indent + 2) else Stigi.Syntax.Print.putLiteral(tables, headed, 3, " = ")
        if Stigi.Syntax.Print.hasItems(body) then Stigi.Syntax.Print.printList(tables, body, JoinedByComma, opened, indent, inner) else headed
      case UnitRoot(_, prompt, decl) =>
        val prompted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 11, "// prompt: "), prompt), 1, "\n")
        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, decl, prompted, 0, inner), 1, "\n")
