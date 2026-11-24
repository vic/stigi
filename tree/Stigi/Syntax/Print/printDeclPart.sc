// prompt: print non-recursive declaration and head parts without spending expression depth.
def Stigi.Syntax.Print.printDeclPart {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen, indent: U64) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  if Stigi.Limit.maxUnitBytes <= indent then PenOverflow else
    Stigi.Syntax.readSlot(tables, node) match
      case MalformedTable(slot) => PenFailed(MalformedNode(slot))
      case Fetched(found) =>
        val Tables(_, _, _, lists, _, _) = tables
        found match
          case Absent(_) | Name(_, _) | Param(_, _, _) | CapParam(_, _, _) => Stigi.Syntax.Print.printParam(tables, node, pen)
          case CapGroup(_, caps) => Stigi.Syntax.Print.printFlatList(tables, caps, InDelimiters(InBraces), pen, indent)
          case FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) => Stigi.Syntax.Print.printFact(tables, node, pen)
          case EnumCase(_, _, _, _, _) => Stigi.Syntax.Print.printEnumCase(tables, node, pen, indent)
          case DefHead(_, typeParams, preGroups, valueGroup, params, postFacts, result, exitFacts) =>
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), pen, indent)
            val Span(groupFirst, groupEnd) = preGroups
            var grouped: Stigi.Syntax.Print.Pen = typed
            var groupIdx: U64 = groupFirst
            while groupIdx < groupEnd && grouped is Pen(_) do
              @<lessOrEqual(groupIdx, groupEnd)>
              val item: U64 = lists(groupIdx)
              val separated: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putSeparator(tables, grouped, EachAfterSpace, groupIdx == groupFirst, indent)
              grouped = if item < Stigi.Limit.maxUnitNodes then
                Stigi.Syntax.readSlot(tables, item) match
                  case MalformedTable(slot) => PenFailed(MalformedNode(slot))
                  case Fetched(groupNode) =>
                    groupNode match
                      case CapGroup(_, caps) => Stigi.Syntax.Print.printFlatList(tables, caps, InDelimiters(InBraces), separated, indent)
                      case Absent(_) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) => Stigi.Syntax.Print.printFact(tables, item, separated)
                      case Name(_, _) | TypeRef(_, _, _, _, _) | Param(_, _, _) | CapParam(_, _, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(item))
              else PenFailed(MalformedNode(groupIdx))
              groupIdx = groupIdx + 1
            val spaced: Stigi.Syntax.Print.Pen = if valueGroup && (Stigi.Syntax.Print.hasItems(typeParams) || Stigi.Syntax.Print.hasItems(preGroups)) then Stigi.Syntax.Print.putLiteral(tables, grouped, 1, " ") else grouped
            val valued: Stigi.Syntax.Print.Pen = if valueGroup then Stigi.Syntax.Print.printFlatList(tables, params, InDelimiters(InParens), spaced, indent) else spaced
            val coloned: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printFactList(tables, postFacts, EachAfterSpace, valued, indent), 1, ":")
            val returned: Stigi.Syntax.Print.Pen = if Stigi.Syntax.Print.isAbsent(tables, result) then coloned else Stigi.Syntax.Print.printType(tables, result, Stigi.Syntax.Print.putLiteral(tables, coloned, 1, " "), Stigi.Limit.maxParseDepth)
            Stigi.Syntax.Print.printFactList(tables, exitFacts, EachAfterSpace, returned, indent)
          case EnumDecl(_, isPublic, name, typeParams, cases) =>
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 5, "enum "), name), indent)
            if Stigi.Limit.maxUnitBytes < indent + 2 then PenOverflow else
              val Span(caseFirst, caseEnd) = cases
              var current: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putLiteral(tables, typed, 1, ":")
              var caseIdx: U64 = caseFirst
              while caseIdx < caseEnd && current is Pen(_) do
                @<lessOrEqual(caseIdx, caseEnd)>
                val item: U64 = lists(caseIdx)
                current = if item < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printEnumCase(tables, item, Stigi.Syntax.Print.putSeparator(tables, current, EachOnLine, caseIdx == caseFirst, indent + 2), indent + 2) else PenFailed(MalformedNode(caseIdx))
                caseIdx = caseIdx + 1
              current
          case TypeDecl(_, isPublic, name, typeParams, valueParams, capParams, target, facts) =>
            val typed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, typeParams, SpacedIn(InBrackets), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 5, "type "), name), indent)
            val capped: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFlatList(tables, capParams, SpacedIn(InBraces), Stigi.Syntax.Print.printFlatList(tables, valueParams, SpacedIn(InParens), typed, indent), indent)
            val targeted: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printType(tables, target, Stigi.Syntax.Print.putLiteral(tables, capped, 3, " = "), Stigi.Limit.maxParseDepth)
            if Stigi.Syntax.Print.isAbsent(tables, facts) then targeted else Stigi.Syntax.Print.printFact(tables, facts, Stigi.Syntax.Print.putLiteral(tables, targeted, 1, " "))
          case RelationDecl(_, isPublic, name, columns) => Stigi.Syntax.Print.printFlatList(tables, columns, InDelimiters(InParens), Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 9, "relation "), name), indent)
          case RuleDecl(_, isPublic, name, head, body, multiline) =>
            val headed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printFact(tables, head, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putPublic(tables, pen, isPublic), 9, "rule def "), name), 2, ": "))
            val opened: Stigi.Syntax.Print.Pen = if multiline then Stigi.Syntax.Print.putSpaces(tables, Stigi.Syntax.Print.putLiteral(tables, headed, 3, " =\n"), indent + 2) else Stigi.Syntax.Print.putLiteral(tables, headed, 3, " = ")
            if Stigi.Syntax.Print.hasItems(body) then Stigi.Syntax.Print.printFactList(tables, body, JoinedByComma, opened, indent) else headed
          case TypeRef(_, _, _, _, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => PenFailed(MalformedNode(node))
