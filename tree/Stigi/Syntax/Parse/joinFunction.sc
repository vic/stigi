// prompt: join a def head, its clauses and its parsed body into a def, a lemma def or an extern def.
def Stigi.Syntax.Parse.joinFunction {Stigi.Mut} (tables: Stigi.Syntax.Tables, isPublic: Bool, kind: Stigi.Syntax.Parse.FunctionKind, name: Stigi.Syntax.Text, head: Stigi.Syntax.NodeAt, clauses: Stigi.Syntax.Span, body: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  body match
    case Failed(fault, at) => Failed(fault, at)
    case Built(bodyNode, after) =>
      Stigi.Syntax.Parse.claimOver(after, head, bodyNode, bodyNode) match
        case Unclaimed(fault) => Failed(fault, head)
        case Claimed(at, headed, bodied, _) =>
          kind match
            case PlainFunction => Stigi.Syntax.Parse.place(tables, at, FunctionDecl(at, isPublic, PlainDef, name, headed, clauses, bodied), after)
            case LemmaFunction => Stigi.Syntax.Parse.place(tables, at, FunctionDecl(at, isPublic, LemmaDef, name, headed, clauses, bodied), after)
            case ExternFunction(abi) => Stigi.Syntax.Parse.place(tables, at, ExternDef(at, isPublic, abi, name, headed, bodied), after)
