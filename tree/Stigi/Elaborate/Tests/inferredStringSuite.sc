// prompt: verify S151 escaped string literal byte length decoding and inferred Array of U8 length in val and call sites.
val Stigi.Elaborate.Tests.inferredStringSuite {Stigi.Test}: Stigi.Test.Result =
  val srcVal: Array[U8](76) = "// prompt: demo.\ndef Demo.msg(): U64 =\n  val buf: Array[U8](_) = \"\\n\\t\"\n  0\n"
  val srcCall: Array[U8](64) = "// prompt: demo.\ndef Demo.call(): U64 =\n  Demo.write(_, \"\\n\\t\")\n"
  val srcBadEsc: Array[U8](50) = "// prompt: demo.\ndef Demo.bad(): U64 =\n  \"\\q\"\n  0\n"
  val srcNonStr: Array[U8](72) = "// prompt: demo.\ndef Demo.bad(): U64 =\n  val buf: Array[U8](_) = 42\n  0\n"
  val rawEsc: Array[U8](6) = "\"\\n\\t\""
  val rawHex: Array[U8](12) = "\"\\x41\\0\\\"\\\\\""
  val rawEmpty: Array[U8](2) = "\"\""
  val rawBadEsc: Array[U8](4) = "\"\\q\""
  val rawShortHex: Array[U8](5) = "\"\\x4\""
  val decPassed: Bool = Stigi.Syntax.stringByteLength(6, rawEsc, 0, 6) is DecodedLength(2) && Stigi.Syntax.stringByteLength(12, rawHex, 0, 12) is DecodedLength(4) && Stigi.Syntax.stringByteLength(2, rawEmpty, 0, 2) is DecodedLength(0) && Stigi.Syntax.stringByteLength(4, rawBadEsc, 0, 4) is MalformedString && Stigi.Syntax.stringByteLength(5, rawShortHex, 0, 5) is MalformedString
  val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
    case AllocationFailed => Stigi.Test.expect(false, true, "syntax table alloc")
  val Filled(exprs) = Array.tryFill[Stigi.Core.Expr]{Stigi.Region}(16, NothingExpr(0)) else
    case FillFailed => Stigi.Test.expect(false, true, "expr alloc")
  val Filled(arms) = Array.tryFill[Stigi.Core.Arm]{Stigi.Region}(4, RestArm(0)) else
    case FillFailed => Stigi.Test.expect(false, true, "arm alloc")
  val Filled(childLists) = Array.tryFill[U64]{Stigi.Region}(8, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "list alloc")
  val ScratchReady(scratch) = Stigi.Elaborate.allocateScratch{Stigi.Region}(128) else
    case ScratchUnavailable => Stigi.Test.expect(false, true, "scratch alloc")
  val Tables(_, _, synNodes, _, _, _) = tables
  val valPassed: Bool = Stigi.Syntax.Parse.parseUnit(tables, 76, srcVal, 76) match
    case ParseRejected(_, _) | LexRejected(_) => false
    case Parsed(_, nodeCount, _) =>
      var foundVal: Bool = false
      var idx: U64 = 0
      while idx < nodeCount && !foundVal do
        @<lessOrEqual(idx, nodeCount)>
        synNodes(idx) match
          case ValStmt(_, _, declared, init, _) =>
            val inferred: Bool = Stigi.Elaborate.inferTypeLength(tables, declared, init) is LengthInferred(2)
            val placed: Bool = Stigi.Elaborate.emitInferredLength(CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 4, arms, 0, 0), 2) match
              case PlaceFull | PlaceInvalid => false
              case PlacedAt(CoreTables(_, _, _, exprCount, _, _, _, listCount, _, _, _, armCount), at) =>
                at == 0 && exprCount == 1 && exprs(0) is IntExpr(0, 2, 0, 0) && listCount <= 8 && armCount <= 4 && Stigi.Core.validateExprTable(16, exprs, exprCount, 4, arms, armCount, 8, childLists, listCount) is TableValid(1)
            foundVal = inferred && placed
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => ()
        idx = idx + 1
      foundVal
  val callPassed: Bool = Stigi.Syntax.Parse.parseUnit(tables, 64, srcCall, 64) match
    case ParseRejected(_, _) | LexRejected(_) => false
    case Parsed(_, nodeCount, _) =>
      var foundCall: Bool = false
      var idx: U64 = 0
      while idx < nodeCount && !foundCall do
        @<lessOrEqual(idx, nodeCount)>
        synNodes(idx) match
          case Call(_, _, _, _, argsSpan, _) =>
            foundCall = Stigi.Elaborate.inferCallArgLength(tables, argsSpan, 0, 1) is LengthInferred(2)
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => ()
        idx = idx + 1
      foundCall
  val nonStrPassed: Bool = Stigi.Syntax.Parse.parseUnit(tables, 72, srcNonStr, 72) match
    case ParseRejected(_, _) | LexRejected(_) => false
    case Parsed(_, nodeCount, _) =>
      var foundBad: Bool = false
      var idx: U64 = 0
      while idx < nodeCount && !foundBad do
        @<lessOrEqual(idx, nodeCount)>
        synNodes(idx) match
          case ValStmt(_, _, declared, init, _) =>
            foundBad = Stigi.Elaborate.inferTypeLength(tables, declared, init) is LengthInferInvalid
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => ()
        idx = idx + 1
      foundBad
  val badEscPassed: Bool = Stigi.Syntax.Parse.parseUnit(tables, 50, srcBadEsc, 50) match
    case ParseRejected(_, _) | LexRejected(_) => false
    case Parsed(_, nodeCount, _) =>
      var foundStr: Bool = false
      var rejectedStr: Bool = false
      var idx: U64 = 0
      while idx < nodeCount && !foundStr do
        @<lessOrEqual(idx, nodeCount)>
        synNodes(idx) match
          case Literal(_, StringLiteral, _) =>
            foundStr = true
            rejectedStr = Stigi.Elaborate.lowerSyntaxNode(tables, idx, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 4, arms, 0, 0), scratch) is LowerInvalid
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, IntLiteral | CharLiteral | TrueLiteral | FalseLiteral | UnitLiteral, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | AliasLine(_, _, _) | FunctionDecl(_, _, _, _, _, _, _) | UnitRoot(_, _, _, _) => ()
        idx = idx + 1
      foundStr && rejectedStr
  Stigi.Test.expect(decPassed && valPassed && callPassed && nonStrPassed && badEscPassed, true, "S151 must decode escaped byte lengths and infer Array[U8](_) and call length args")
