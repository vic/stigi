// prompt: lex and parse the first bytes of a text buffer as one unit into the syntax tables: its prompt line, its alias lines and its one declaration.
pub def Stigi.Syntax.Parse.parseUnit {Stigi.Mut} (tables: Stigi.Syntax.Tables, textCap: U64, text: Array[U8](textCap), size: U64) <lessOrEqual(size, textCap), lessOrEqual(size, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Outcome =
  Stigi.Syntax.Parse.lexBuffer(tables, textCap, text, size) match
    case LexOverflow => LexRejected(size)
    case LexSyntaxError(offset, _) => LexRejected(offset)
    case LexDone(count) =>
      Stigi.Syntax.Parse.promptOf(tables, size) match
        case PromptAbsent => ParseRejected(UnexpectedToken, 0)
        case PromptAt(prompt) =>
          Stigi.Syntax.Parse.readAliases(tables, Progress(Stigi.Syntax.Parse.lineStart(tables, 0, Stigi.Limit.maxUnitBytes), 0, 0, 0)) match
            case ListFailed(fault, at) => ParseRejected(fault, at)
            case Listed(aliases, aliased) =>
              Stigi.Syntax.Parse.parseDecl(tables, aliased, 0, Stigi.Limit.maxParseDepth) match
                case Failed(fault, at) => ParseRejected(fault, at)
                case Built(decl, after) =>
                  val Progress(rest, _, _, _) = after
                  if !(Stigi.Syntax.Parse.kindAt(tables, Stigi.Syntax.Parse.lineStart(tables, rest, Stigi.Limit.maxUnitBytes)) is TokEnd) then ParseRejected(UnexpectedToken, rest) else
                    Stigi.Syntax.Parse.claimOver(after, decl, decl, decl) match
                      case Unclaimed(fault) => ParseRejected(fault, rest)
                      case Claimed(at, declared, _, _) =>
                        Stigi.Syntax.Parse.place(tables, at, UnitRoot(at, prompt, aliases, declared), after) match
                          case Failed(fault, token) => ParseRejected(fault, token)
                          case Built(root, _) => Parsed(root, at + 1, count)
