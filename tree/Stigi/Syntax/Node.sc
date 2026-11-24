// prompt: one node of the postorder syntax table of a unit: each node holds its own index and every child index is below it.
enum Stigi.Syntax.Node:
  case Absent(at: Stigi.Syntax.NodeAt)
  case Name(at: Stigi.Syntax.NodeAt, text: Stigi.Syntax.Text)
  case TypeRef(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, typeArgs: Stigi.Syntax.Span, lengthArgs: Stigi.Syntax.Span, capArg: Stigi.Syntax.NodeAt) <below(capArg, at)>
  case FactAtom(at: Stigi.Syntax.NodeAt, negated: Bool, evidence: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, args: Stigi.Syntax.Span) <below(evidence, at)>
  case FactCompare(at: Stigi.Syntax.NodeAt, negated: Bool, evidence: Stigi.Syntax.NodeAt, left: Stigi.Syntax.NodeAt, operator: Stigi.Syntax.BinaryOperator, right: Stigi.Syntax.NodeAt) <below(evidence, at), below(left, at), below(right, at)>
  case FactArith(at: Stigi.Syntax.NodeAt, negated: Bool, evidence: Stigi.Syntax.NodeAt, target: Stigi.Syntax.Text, left: Stigi.Syntax.NodeAt, operator: Stigi.Syntax.BinaryOperator, right: Stigi.Syntax.NodeAt) <below(evidence, at), below(left, at), below(right, at)>
  case FactDeny(at: Stigi.Syntax.NodeAt, items: Stigi.Syntax.Span)
  case FactGroup(at: Stigi.Syntax.NodeAt, items: Stigi.Syntax.Span)
  case Param(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, declared: Stigi.Syntax.NodeAt) <below(declared, at)>
  case CapParam(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.NodeAt, declared: Stigi.Syntax.NodeAt) <below(name, at), below(declared, at)>
  case CapGroup(at: Stigi.Syntax.NodeAt, caps: Stigi.Syntax.Span)
  case PatName(at: Stigi.Syntax.NodeAt, text: Stigi.Syntax.Text)
  case PatLiteral(at: Stigi.Syntax.NodeAt, text: Stigi.Syntax.Text)
  case PatCtor(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, parts: Stigi.Syntax.Span)
  case PatTuple(at: Stigi.Syntax.NodeAt, parts: Stigi.Syntax.Span)
  case PatOr(at: Stigi.Syntax.NodeAt, alternatives: Stigi.Syntax.Span)
  case PatRange(at: Stigi.Syntax.NodeAt, low: Stigi.Syntax.NodeAt, high: Stigi.Syntax.NodeAt) <below(low, at), below(high, at)>
  case Literal(at: Stigi.Syntax.NodeAt, kind: Stigi.Syntax.LiteralKind, text: Stigi.Syntax.Text)
  case Ref(at: Stigi.Syntax.NodeAt, text: Stigi.Syntax.Text)
  case Call(at: Stigi.Syntax.NodeAt, callee: Stigi.Syntax.NodeAt, typeArgs: Stigi.Syntax.Span, capArgs: Stigi.Syntax.Span, args: Stigi.Syntax.Span, parens: Bool) <below(callee, at)>
  case Binary(at: Stigi.Syntax.NodeAt, operator: Stigi.Syntax.BinaryOperator, left: Stigi.Syntax.NodeAt, right: Stigi.Syntax.NodeAt) <below(left, at), below(right, at)>
  case Unary(at: Stigi.Syntax.NodeAt, operator: Stigi.Syntax.UnaryOperator, operand: Stigi.Syntax.NodeAt) <below(operand, at)>
  case IsTest(at: Stigi.Syntax.NodeAt, subject: Stigi.Syntax.NodeAt, pattern: Stigi.Syntax.NodeAt) <below(subject, at), below(pattern, at)>
  case IfElse(at: Stigi.Syntax.NodeAt, condition: Stigi.Syntax.NodeAt, whenTrue: Stigi.Syntax.NodeAt, whenFalse: Stigi.Syntax.NodeAt) <below(condition, at), below(whenTrue, at), below(whenFalse, at)>
  case WhileLoop(at: Stigi.Syntax.NodeAt, condition: Stigi.Syntax.NodeAt, body: Stigi.Syntax.NodeAt) <below(condition, at), below(body, at)>
  case MatchOn(at: Stigi.Syntax.NodeAt, scrutinees: Stigi.Syntax.Span, arms: Stigi.Syntax.Span)
  case Arm(at: Stigi.Syntax.NodeAt, pattern: Stigi.Syntax.NodeAt, guard: Stigi.Syntax.NodeAt, body: Stigi.Syntax.NodeAt) <below(pattern, at), below(guard, at), below(body, at)>
  case ValStmt(at: Stigi.Syntax.NodeAt, pattern: Stigi.Syntax.NodeAt, declared: Stigi.Syntax.NodeAt, init: Stigi.Syntax.NodeAt, elseArms: Stigi.Syntax.Span) <below(pattern, at), below(declared, at), below(init, at)>
  case VarStmt(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, invariant: Stigi.Syntax.NodeAt, declared: Stigi.Syntax.NodeAt, init: Stigi.Syntax.NodeAt) <below(invariant, at), below(declared, at), below(init, at)>
  case AssignStmt(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, value: Stigi.Syntax.NodeAt) <below(value, at)>
  case IndexRead(at: Stigi.Syntax.NodeAt, target: Stigi.Syntax.NodeAt, index: Stigi.Syntax.NodeAt) <below(target, at), below(index, at)>
  case IndexWrite(at: Stigi.Syntax.NodeAt, target: Stigi.Syntax.NodeAt, index: Stigi.Syntax.NodeAt, value: Stigi.Syntax.NodeAt) <below(target, at), below(index, at), below(value, at)>
  case HandleStmt(at: Stigi.Syntax.NodeAt, binder: Stigi.Syntax.NodeAt, invoked: Stigi.Syntax.Text, typeArgs: Stigi.Syntax.Span, capArgs: Stigi.Syntax.Span, args: Stigi.Syntax.Span, parens: Bool, body: Stigi.Syntax.NodeAt) <below(binder, at), below(body, at)>
  case AssertStmt(at: Stigi.Syntax.NodeAt, subject: Stigi.Syntax.NodeAt, facts: Stigi.Syntax.NodeAt) <below(subject, at), below(facts, at)>
  case DenyStmt(at: Stigi.Syntax.NodeAt, item: Stigi.Syntax.NodeAt) <below(item, at)>
  case Block(at: Stigi.Syntax.NodeAt, statements: Stigi.Syntax.Span)
  case LocalDecl(at: Stigi.Syntax.NodeAt, decl: Stigi.Syntax.NodeAt) <below(decl, at)>
  case DefHead(at: Stigi.Syntax.NodeAt, typeParams: Stigi.Syntax.Span, preGroups: Stigi.Syntax.Span, valueGroup: Bool, params: Stigi.Syntax.Span, postFacts: Stigi.Syntax.Span, returns: Stigi.Syntax.NodeAt, exitFacts: Stigi.Syntax.Span) <below(returns, at)>
  case FunctionDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, flavor: Stigi.Syntax.DefFlavor, name: Stigi.Syntax.Text, head: Stigi.Syntax.NodeAt, clauses: Stigi.Syntax.Span, body: Stigi.Syntax.NodeAt) <below(head, at), below(body, at)>
  case ValDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, caps: Stigi.Syntax.Span, declared: Stigi.Syntax.NodeAt, body: Stigi.Syntax.NodeAt) <below(declared, at), below(body, at)>
  case EnumDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, cases: Stigi.Syntax.Span)
  case EnumCase(at: Stigi.Syntax.NodeAt, name: Stigi.Syntax.Text, parens: Bool, fields: Stigi.Syntax.Span, facts: Stigi.Syntax.NodeAt) <below(facts, at)>
  case TypeDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, valueParams: Stigi.Syntax.Span, capParams: Stigi.Syntax.Span, target: Stigi.Syntax.NodeAt, facts: Stigi.Syntax.NodeAt) <below(target, at), below(facts, at)>
  case CapabilityDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, states: Stigi.Syntax.Span, startState: Stigi.Syntax.NodeAt, stops: Stigi.Syntax.Span, members: Stigi.Syntax.Span) <below(startState, at)>
  case HandlerDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, capParams: Stigi.Syntax.Span, valueGroup: Bool, valueParams: Stigi.Syntax.Span, target: Stigi.Syntax.NodeAt, vars: Stigi.Syntax.Span, defs: Stigi.Syntax.Span) <below(target, at)>
  case TraitDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, members: Stigi.Syntax.Span)
  case GivenDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, typeParams: Stigi.Syntax.Span, capParams: Stigi.Syntax.Span, target: Stigi.Syntax.NodeAt, defs: Stigi.Syntax.Span) <below(target, at)>
  case ExternDef(at: Stigi.Syntax.NodeAt, isPublic: Bool, abi: Stigi.Syntax.Text, name: Stigi.Syntax.Text, head: Stigi.Syntax.NodeAt, body: Stigi.Syntax.NodeAt) <below(head, at), below(body, at)>
  case ExternVal(at: Stigi.Syntax.NodeAt, isPublic: Bool, abi: Stigi.Syntax.Text, name: Stigi.Syntax.Text, declared: Stigi.Syntax.NodeAt) <below(declared, at)>
  case RelationDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, columns: Stigi.Syntax.Span)
  case RuleDecl(at: Stigi.Syntax.NodeAt, isPublic: Bool, name: Stigi.Syntax.Text, head: Stigi.Syntax.NodeAt, body: Stigi.Syntax.Span, multiline: Bool) <below(head, at)>
  case UnitRoot(at: Stigi.Syntax.NodeAt, prompt: Stigi.Syntax.Text, decl: Stigi.Syntax.NodeAt) <below(decl, at)>
