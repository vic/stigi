// prompt: print the syntax node at one slot in canonical layout with O(1) iterative left spines and non-recursive declaration and fact printing.
pub def Stigi.Syntax.Print.printNode {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Limit.maxUnitBytes <= indent then PenOverflow else
    val inner: U64 = depth - 1
    Stigi.Syntax.readSlot(tables, node) match
      case MalformedTable(slot) => PenFailed(MalformedNode(slot))
      case Fetched(found) =>
        found match
          case Absent(_) => pen
          case Name(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
          case TypeRef(_, _, _, _, _) => Stigi.Syntax.Print.printType(tables, node, pen, Stigi.Limit.maxParseDepth)
          case FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) => Stigi.Syntax.Print.printFact(tables, node, pen)
          case Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | DefHead(_, _, _, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) => Stigi.Syntax.Print.printDeclPart(tables, node, pen, indent)
          case PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) => Stigi.Syntax.Print.printPattern(tables, node, pen, Stigi.Limit.maxParseDepth)
          case Literal(_, kind, text) => if kind is UnitLiteral then Stigi.Syntax.Print.putLiteral(tables, pen, 2, "()") else Stigi.Syntax.Print.putText(tables, pen, text)
          case Ref(_, text) => Stigi.Syntax.Print.putText(tables, pen, text)
          case Binary(_, operator, left, right) =>
            if Stigi.Syntax.Print.isSpineChild(tables, left) then
              Stigi.Syntax.Print.walkSpine(tables, node, pen) match
                case SpineBad(errPen) => errPen
                case SpineBare(facts) => Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "@"))
                case SpineChain(leaf, total, parenPen) =>
                  var outPen: Stigi.Syntax.Print.Pen = if leaf < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, leaf, parenPen, indent, inner) else PenFailed(MalformedNode(node))
                  var remaining: U64 = if total <= Stigi.Limit.maxUnitNodes then total else 0
                  while 0 < remaining && outPen is Pen(_) do
                    @<lessOrEqual(remaining, Stigi.Limit.maxUnitNodes)>
                    remaining = remaining - 1
                    outPen = Stigi.Syntax.Print.findSpineStep(tables, node, remaining) match
                      case MissingSpine(badSlot) => PenFailed(MalformedNode(badSlot))
                      case EmitBinary(operator, right, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), Stigi.Syntax.Print.precedence(operator), true)
                        val rightLead: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putBinary(tables, closed, operator)
                        val rightStart: Stigi.Syntax.Print.Pen = if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightLead, 1, "(") else rightLead
                        val rightMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, right, rightStart, indent, inner)
                        if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightMid, 1, ")") else rightMid
                      case EmitIs(pattern, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, closed, 4, " is "), Stigi.Limit.maxParseDepth)
                      case EmitCall(typeArgs, capArgs, args, parens, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), closed, Stigi.Limit.maxParseDepth)
                        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
                        if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
                      case EmitIndex(index, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "("), indent, inner), 1, ")")
                      case EmitAssert(facts, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "@"))
                  outPen
            else
              val outer: U64 = Stigi.Syntax.Print.precedence(operator)
              val leftParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, left), outer, false)
              val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), outer, true)
              val leftStart: Stigi.Syntax.Print.Pen = if leftParen then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
              val leftMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, left, leftStart, indent, inner)
              val lefted: Stigi.Syntax.Print.Pen = if leftParen then Stigi.Syntax.Print.putLiteral(tables, leftMid, 1, ")") else leftMid
              val rightLead: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putBinary(tables, lefted, operator)
              val rightStart: Stigi.Syntax.Print.Pen = if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightLead, 1, "(") else rightLead
              val rightMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, right, rightStart, indent, inner)
              if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightMid, 1, ")") else rightMid
          case IsTest(_, subject, pattern) =>
            if Stigi.Syntax.Print.isSpineChild(tables, subject) then
              Stigi.Syntax.Print.walkSpine(tables, node, pen) match
                case SpineBad(errPen) => errPen
                case SpineBare(facts) => Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "@"))
                case SpineChain(leaf, total, parenPen) =>
                  var outPen: Stigi.Syntax.Print.Pen = if leaf < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, leaf, parenPen, indent, inner) else PenFailed(MalformedNode(node))
                  var remaining: U64 = if total <= Stigi.Limit.maxUnitNodes then total else 0
                  while 0 < remaining && outPen is Pen(_) do
                    @<lessOrEqual(remaining, Stigi.Limit.maxUnitNodes)>
                    remaining = remaining - 1
                    outPen = Stigi.Syntax.Print.findSpineStep(tables, node, remaining) match
                      case MissingSpine(badSlot) => PenFailed(MalformedNode(badSlot))
                      case EmitBinary(operator, right, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), Stigi.Syntax.Print.precedence(operator), true)
                        val rightLead: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putBinary(tables, closed, operator)
                        val rightStart: Stigi.Syntax.Print.Pen = if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightLead, 1, "(") else rightLead
                        val rightMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, right, rightStart, indent, inner)
                        if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightMid, 1, ")") else rightMid
                      case EmitIs(pattern, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, closed, 4, " is "), Stigi.Limit.maxParseDepth)
                      case EmitCall(typeArgs, capArgs, args, parens, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), closed, Stigi.Limit.maxParseDepth)
                        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
                        if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
                      case EmitIndex(index, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "("), indent, inner), 1, ")")
                      case EmitAssert(facts, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "@"))
                  outPen
            else
              val subjParen: Bool = Stigi.Syntax.Print.subjectParens(Stigi.Syntax.Print.shapeOf(tables, subject))
              val subjStart: Stigi.Syntax.Print.Pen = if subjParen then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
              val subjMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, subject, subjStart, indent, inner)
              val subjed: Stigi.Syntax.Print.Pen = if subjParen then Stigi.Syntax.Print.putLiteral(tables, subjMid, 1, ")") else subjMid
              Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, subjed, 4, " is "), Stigi.Limit.maxParseDepth)
          case Call(_, callee, typeArgs, capArgs, args, parens) =>
            if Stigi.Syntax.Print.isSpineChild(tables, callee) then
              Stigi.Syntax.Print.walkSpine(tables, node, pen) match
                case SpineBad(errPen) => errPen
                case SpineBare(facts) => Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "@"))
                case SpineChain(leaf, total, parenPen) =>
                  var outPen: Stigi.Syntax.Print.Pen = if leaf < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, leaf, parenPen, indent, inner) else PenFailed(MalformedNode(node))
                  var remaining: U64 = if total <= Stigi.Limit.maxUnitNodes then total else 0
                  while 0 < remaining && outPen is Pen(_) do
                    @<lessOrEqual(remaining, Stigi.Limit.maxUnitNodes)>
                    remaining = remaining - 1
                    outPen = Stigi.Syntax.Print.findSpineStep(tables, node, remaining) match
                      case MissingSpine(badSlot) => PenFailed(MalformedNode(badSlot))
                      case EmitBinary(operator, right, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), Stigi.Syntax.Print.precedence(operator), true)
                        val rightLead: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putBinary(tables, closed, operator)
                        val rightStart: Stigi.Syntax.Print.Pen = if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightLead, 1, "(") else rightLead
                        val rightMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, right, rightStart, indent, inner)
                        if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightMid, 1, ")") else rightMid
                      case EmitIs(pattern, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, closed, 4, " is "), Stigi.Limit.maxParseDepth)
                      case EmitCall(typeArgs, capArgs, args, parens, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), closed, Stigi.Limit.maxParseDepth)
                        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
                        if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
                      case EmitIndex(index, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "("), indent, inner), 1, ")")
                      case EmitAssert(facts, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "@"))
                  outPen
            else
              val calleeParen: Bool = Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, callee))
              val calleeStart: Stigi.Syntax.Print.Pen = if calleeParen then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
              val calleeMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, callee, calleeStart, indent, inner)
              val called: Stigi.Syntax.Print.Pen = if calleeParen then Stigi.Syntax.Print.putLiteral(tables, calleeMid, 1, ")") else calleeMid
              val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), called, Stigi.Limit.maxParseDepth)
              val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
              if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
          case IndexRead(_, target, index) =>
            if Stigi.Syntax.Print.isSpineChild(tables, target) then
              Stigi.Syntax.Print.walkSpine(tables, node, pen) match
                case SpineBad(errPen) => errPen
                case SpineBare(facts) => Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "@"))
                case SpineChain(leaf, total, parenPen) =>
                  var outPen: Stigi.Syntax.Print.Pen = if leaf < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, leaf, parenPen, indent, inner) else PenFailed(MalformedNode(node))
                  var remaining: U64 = if total <= Stigi.Limit.maxUnitNodes then total else 0
                  while 0 < remaining && outPen is Pen(_) do
                    @<lessOrEqual(remaining, Stigi.Limit.maxUnitNodes)>
                    remaining = remaining - 1
                    outPen = Stigi.Syntax.Print.findSpineStep(tables, node, remaining) match
                      case MissingSpine(badSlot) => PenFailed(MalformedNode(badSlot))
                      case EmitBinary(operator, right, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), Stigi.Syntax.Print.precedence(operator), true)
                        val rightLead: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putBinary(tables, closed, operator)
                        val rightStart: Stigi.Syntax.Print.Pen = if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightLead, 1, "(") else rightLead
                        val rightMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, right, rightStart, indent, inner)
                        if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightMid, 1, ")") else rightMid
                      case EmitIs(pattern, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, closed, 4, " is "), Stigi.Limit.maxParseDepth)
                      case EmitCall(typeArgs, capArgs, args, parens, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), closed, Stigi.Limit.maxParseDepth)
                        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
                        if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
                      case EmitIndex(index, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "("), indent, inner), 1, ")")
                      case EmitAssert(facts, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "@"))
                  outPen
            else
              val targetParen: Bool = Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, target))
              val targetStart: Stigi.Syntax.Print.Pen = if targetParen then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
              val targetMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, target, targetStart, indent, inner)
              val targeted: Stigi.Syntax.Print.Pen = if targetParen then Stigi.Syntax.Print.putLiteral(tables, targetMid, 1, ")") else targetMid
              Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, "("), indent, inner), 1, ")")
          case AssertStmt(_, subject, facts) =>
            if Stigi.Syntax.Print.isAbsent(tables, subject) then Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "@")) else if Stigi.Syntax.Print.isSpineChild(tables, subject) then
              Stigi.Syntax.Print.walkSpine(tables, node, pen) match
                case SpineBad(errPen) => errPen
                case SpineBare(facts) => Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "@"))
                case SpineChain(leaf, total, parenPen) =>
                  var outPen: Stigi.Syntax.Print.Pen = if leaf < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, leaf, parenPen, indent, inner) else PenFailed(MalformedNode(node))
                  var remaining: U64 = if total <= Stigi.Limit.maxUnitNodes then total else 0
                  while 0 < remaining && outPen is Pen(_) do
                    @<lessOrEqual(remaining, Stigi.Limit.maxUnitNodes)>
                    remaining = remaining - 1
                    outPen = Stigi.Syntax.Print.findSpineStep(tables, node, remaining) match
                      case MissingSpine(badSlot) => PenFailed(MalformedNode(badSlot))
                      case EmitBinary(operator, right, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val rightParen: Bool = Stigi.Syntax.Print.sideParens(Stigi.Syntax.Print.shapeOf(tables, right), Stigi.Syntax.Print.precedence(operator), true)
                        val rightLead: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putBinary(tables, closed, operator)
                        val rightStart: Stigi.Syntax.Print.Pen = if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightLead, 1, "(") else rightLead
                        val rightMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, right, rightStart, indent, inner)
                        if rightParen then Stigi.Syntax.Print.putLiteral(tables, rightMid, 1, ")") else rightMid
                      case EmitIs(pattern, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, closed, 4, " is "), Stigi.Limit.maxParseDepth)
                      case EmitCall(typeArgs, capArgs, args, parens, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), closed, Stigi.Limit.maxParseDepth)
                        val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
                        if parens || Stigi.Syntax.Print.hasItems(args) then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
                      case EmitIndex(index, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "("), indent, inner), 1, ")")
                      case EmitAssert(facts, wrapClose) =>
                        val closed: Stigi.Syntax.Print.Pen = if wrapClose then Stigi.Syntax.Print.putLiteral(tables, outPen, 1, ")") else outPen
                        Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, closed, 1, "@"))
                  outPen
            else
              val subjParen: Bool = Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, subject))
              val subjStart: Stigi.Syntax.Print.Pen = if subjParen then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
              val subjMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, subject, subjStart, indent, inner)
              val subjed: Stigi.Syntax.Print.Pen = if subjParen then Stigi.Syntax.Print.putLiteral(tables, subjMid, 1, ")") else subjMid
              Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, subjed, 1, "@"))
          case Unary(_, operator, operand) =>
            val operParen: Bool = Stigi.Syntax.Print.operandParens(Stigi.Syntax.Print.shapeOf(tables, operand), operator is Negate)
            val leadPen: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putUnary(tables, pen, operator)
            val operStart: Stigi.Syntax.Print.Pen = if operParen then Stigi.Syntax.Print.putLiteral(tables, leadPen, 1, "(") else leadPen
            val operMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, operand, operStart, indent, inner)
            if operParen then Stigi.Syntax.Print.putLiteral(tables, operMid, 1, ")") else operMid
          case IfElse(_, condition, whenTrue, whenFalse) =>
            val tested: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, condition, Stigi.Syntax.Print.putLiteral(tables, pen, 3, "if "), indent, inner), 5, " then")
            val thenDone: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printBody(tables, whenTrue, tested, indent, inner)
            val elsed: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isMultiline(tables, whenTrue, Stigi.Limit.maxParseDepth) then Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putNewline(tables, thenDone, indent), 4, "else") else Stigi.Syntax.Print.putLiteral(tables, thenDone, 5, " else")
            Stigi.Syntax.Print.printBody(tables, whenFalse, elsed, indent, inner)
          case WhileLoop(_, condition, body) => Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, condition, Stigi.Syntax.Print.putLiteral(tables, pen, 6, "while "), indent, inner), 3, " do"), indent, inner)
          case MatchOn(_, scrutinees, arms) =>
            val Span(scrutFirst, scrutEnd) = scrutinees
            val Tables(_, _, _, lists, _, _) = tables
            val scruted: Stigi.Syntax.Print.Pen = if scrutFirst < scrutEnd && scrutEnd - scrutFirst < 2 then
              val single: U64 = lists(scrutFirst)
              if single < Stigi.Limit.maxUnitNodes then
                val wrapSingle: Bool = Stigi.Syntax.Print.scrutineeParens(Stigi.Syntax.Print.shapeOf(tables, single))
                val openSingle: Stigi.Syntax.Print.Pen = if wrapSingle then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
                val midSingle: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, single, openSingle, indent, inner)
                if wrapSingle then Stigi.Syntax.Print.putLiteral(tables, midSingle, 1, ")") else midSingle
              else PenFailed(MalformedNode(scrutFirst))
            else Stigi.Syntax.Print.printList(tables, scrutinees, InDelimiters(InParens), pen, indent, inner)
            Stigi.Syntax.Print.printList(tables, arms, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, scruted, 6, " match"), indent + 2, inner)
          case Arm(_, pattern, guard, body) =>
            val patPen: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, pen, 5, "case "), Stigi.Limit.maxParseDepth)
            val guarded: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, guard) then patPen else Stigi.Syntax.Print.printNode(tables, guard, Stigi.Syntax.Print.putLiteral(tables, patPen, 4, " if "), indent, inner)
            Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, guarded, 3, " =>"), indent, inner)
          case ValStmt(_, pattern, declared, init, elseArms) =>
            val patPen: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printPattern(tables, pattern, Stigi.Syntax.Print.putLiteral(tables, pen, 4, "val "), Stigi.Limit.maxParseDepth)
            val typed: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, declared) then patPen else Stigi.Syntax.Print.printType(tables, declared, Stigi.Syntax.Print.putLiteral(tables, patPen, 2, ": "), Stigi.Limit.maxParseDepth)
            val valued: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printBody(tables, init, Stigi.Syntax.Print.putLiteral(tables, typed, 2, " ="), indent, inner)
            if Stigi.Syntax.Print.hasItems(elseArms) then Stigi.Syntax.Print.printList(tables, elseArms, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, valued, 5, " else"), indent + 2, inner) else valued
          case VarStmt(_, name, invariant, declared, init) =>
            val named: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 4, "var "), name)
            val held: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, invariant) then named else Stigi.Syntax.Print.printFact(tables, invariant, Stigi.Syntax.Print.putLiteral(tables, named, 1, "@"))
            val typed: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, declared) then held else Stigi.Syntax.Print.printType(tables, declared, Stigi.Syntax.Print.putLiteral(tables, held, 2, ": "), Stigi.Limit.maxParseDepth)
            Stigi.Syntax.Print.printBody(tables, init, Stigi.Syntax.Print.putLiteral(tables, typed, 2, " ="), indent, inner)
          case AssignStmt(_, name, value) => Stigi.Syntax.Print.printBody(tables, value, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, pen, name), 2, " ="), indent, inner)
          case IndexWrite(_, target, index, value) =>
            val targetParen: Bool = Stigi.Syntax.Print.atomParens(Stigi.Syntax.Print.shapeOf(tables, target))
            val targetStart: Stigi.Syntax.Print.Pen = if targetParen then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
            val targetMid: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, target, targetStart, indent, inner)
            val targeted: Stigi.Syntax.Print.Pen = if targetParen then Stigi.Syntax.Print.putLiteral(tables, targetMid, 1, ")") else targetMid
            val indexed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, index, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, "("), indent, inner), 4, ") :=")
            Stigi.Syntax.Print.printBody(tables, value, indexed, indent, inner)
          case HandleStmt(_, binder, invoked, typeArgs, capArgs, args, parens, body) =>
            val started: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, pen, 7, "handle ")
            val bound: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, binder) then started else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, binder, started, indent, inner), 3, " = ")
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTypeList(tables, typeArgs, WrappedIn(InBrackets), Stigi.Syntax.Print.putText(tables, bound, invoked), Stigi.Limit.maxParseDepth)
            val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printTermList(tables, capArgs, WrappedIn(InBraces), typed)
            val called: Stigi.Syntax.Print.Pen = if parens then Stigi.Syntax.Print.printList(tables, args, InDelimiters(InParens), capped, indent, inner) else capped
            if Stigi.Syntax.Print.isAbsent(tables, body) then called else Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, called, 1, ":"), indent, inner)
          case DenyStmt(_, item) => Stigi.Syntax.Print.printFact(tables, item, pen)
          case Block(_, statements) => Stigi.Syntax.Print.printList(tables, statements, StackedLines, pen, indent, inner)
          case LocalDecl(_, decl) => Stigi.Syntax.Print.printNode(tables, decl, pen, indent, inner)
          case FunctionDecl(_, isPublic, flavor, name, head, clauses, body) =>
            val headed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printDeclPart(tables, head, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putFlavor(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), flavor), name), indent)
            if Stigi.Syntax.Print.hasItems(clauses) then Stigi.Syntax.Print.printList(tables, clauses, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, headed, 2, " ="), indent + 2, inner) else if Stigi.Syntax.Print.isAbsent(tables, body) then headed else Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, headed, 2, " ="), indent, inner)
          case ValDecl(_, isPublic, name, caps, declared, body) =>
            val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, caps, SpacedIn(InBraces), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 4, "val "), name), indent)
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printType(tables, declared, Stigi.Syntax.Print.putLiteral(tables, capped, 2, ": "), Stigi.Limit.maxParseDepth)
            Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, typed, 2, " ="), indent, inner)
          case CapabilityDecl(_, isPublic, name, typeParams, states, startState, stops, members) =>
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 11, "capability "), name), indent)
            val stated: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printStates(tables, states, startState, stops, Stigi.Syntax.Print.putLiteral(tables, typed, 1, ":"), indent + 2, inner)
            Stigi.Syntax.Print.printList(tables, members, EachOnLine, stated, indent + 2, inner)
          case HandlerDecl(_, isPublic, name, typeParams, capParams, valueGroup, valueParams, target, vars, defs) =>
            val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, capParams, SpacedIn(InBraces), Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 8, "handler "), name), indent), indent)
            val spaced: Stigi.Syntax.Print.Pen = if valueGroup && (Stigi.Syntax.Print.hasItems(typeParams) || Stigi.Syntax.Print.hasItems(capParams)) then Stigi.Syntax.Print.putLiteral(tables, capped, 1, " ") else capped
            val valued: Stigi.Syntax.Print.Pen = if valueGroup then Stigi.Syntax.Print.printFlatList(tables, valueParams, InDelimiters(InParens), spaced, indent) else spaced
            val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printType(tables, target, Stigi.Syntax.Print.putLiteral(tables, valued, 5, " for "), Stigi.Limit.maxParseDepth)
            Stigi.Syntax.Print.printList(tables, defs, EachOnLine, Stigi.Syntax.Print.printList(tables, vars, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, ":"), indent + 2, inner), indent + 2, inner)
          case TraitDecl(_, isPublic, name, typeParams, members) =>
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 6, "trait "), name), indent)
            Stigi.Syntax.Print.printList(tables, members, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, typed, 1, ":"), indent + 2, inner)
          case GivenDecl(_, isPublic, name, typeParams, capParams, target, defs) =>
            val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, capParams, SpacedIn(InBraces), Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 6, "given "), name), indent), indent)
            val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printType(tables, target, Stigi.Syntax.Print.putLiteral(tables, capped, 2, ": "), Stigi.Limit.maxParseDepth)
            Stigi.Syntax.Print.printList(tables, defs, EachOnLine, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, ":"), indent + 2, inner)
          case ExternDef(_, isPublic, abi, name, head, body) =>
            val named: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 7, "extern "), abi), 5, " def "), name)
            val headed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printDeclPart(tables, head, named, indent)
            if Stigi.Syntax.Print.isAbsent(tables, body) then headed else Stigi.Syntax.Print.printBody(tables, body, Stigi.Syntax.Print.putLiteral(tables, headed, 2, " ="), indent, inner)
          case ExternVal(_, isPublic, abi, name, declared) =>
            val named: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 7, "extern "), abi), 5, " val "), name)
            Stigi.Syntax.Print.printType(tables, declared, Stigi.Syntax.Print.putLiteral(tables, named, 2, ": "), Stigi.Limit.maxParseDepth)
          case UnitRoot(_, prompt, aliases, decl) =>
            val prompted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 11, "// prompt: "), prompt)
            val aliased: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printList(tables, aliases, EachOnLine, prompted, 0, inner)
            Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, decl, Stigi.Syntax.Print.putLiteral(tables, aliased, 1, "\n"), 0, inner), 1, "\n")
          case AliasLine(_, name, target) => Stigi.Syntax.Print.printNode(tables, target, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, name, Stigi.Syntax.Print.putLiteral(tables, pen, 6, "alias "), indent, inner), 3, " = "), indent, inner)
