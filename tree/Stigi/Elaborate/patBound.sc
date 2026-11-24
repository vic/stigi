// prompt: read the signed integer bound written by one literal pattern node, a character as its byte value, or a bad literal for any other node.
def Stigi.Elaborate.patBound {Stigi.Mut} (tables: Stigi.Syntax.Tables, boundSyn: Stigi.Syntax.NodeAt, mapCap: U64) <below(boundSyn, mapCap)>: Stigi.Elaborate.ParsedInt =
  val Tables(source, _, synNodes, _, _, _) = tables
  synNodes(boundSyn) match
    case PatLiteral(_, litText) =>
      litText match
        case Text(textFirst, textEnd) =>
          Stigi.Syntax.literalKindOfText(Stigi.Limit.maxUnitBytes, source, textFirst, textEnd) match
            case IntLiteral => Stigi.Elaborate.parseSignedLiteral(Stigi.Limit.maxUnitBytes, source, textFirst, textEnd)
            case CharLiteral =>
              Stigi.Syntax.charValue(Stigi.Limit.maxUnitBytes, source, textFirst, textEnd) match
                case CharMalformed => BadIntLiteral
                case CharDecoded(charVal) => ParsedInt(I64.ofU64(charVal))
            case StringLiteral | TrueLiteral | FalseLiteral | UnitLiteral => BadIntLiteral
    case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => BadIntLiteral
