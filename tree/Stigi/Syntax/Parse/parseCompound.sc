// prompt: parse a capability, handler, trait or given from its name token: its head, then its member lines.
def Stigi.Syntax.Parse.parseCompound {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool, kind: Stigi.Syntax.Parse.CompoundKind, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if depth < 1 then Failed(TooDeep, token) else
    val head: Stigi.Syntax.Parse.CompoundHead = Stigi.Syntax.Parse.readCompoundHead(tables, start, kind)
    head match
      case CompoundHeadFailed(fault, at) => Failed(fault, at)
      case CompoundHead(_, _, _, _, _, _, afterHead) =>
        kind match
          case IsCapability =>
            val states: Stigi.Syntax.Parse.States = Stigi.Syntax.Parse.readStates(tables, afterHead)
            states match
              case StatesFailed(fault, at) => Failed(fault, at)
              case States(_, _, _, afterStates) => Stigi.Syntax.Parse.joinCapability(tables, isPublic, head, states, Stigi.Syntax.Parse.parseMembers(tables, afterStates, DeclMembers, depth - 1))
          case IsHandler =>
            Stigi.Syntax.Parse.parseMembers(tables, afterHead, VarMembers, depth - 1) match
              case ListFailed(fault, at) => Failed(fault, at)
              case Listed(vars, afterVars) => Stigi.Syntax.Parse.joinHandler(tables, isPublic, head, vars, Stigi.Syntax.Parse.parseMembers(tables, afterVars, DeclMembers, depth - 1))
          case IsTrait | IsGiven => Stigi.Syntax.Parse.joinMembers(tables, isPublic, kind, head, Stigi.Syntax.Parse.parseMembers(tables, afterHead, DeclMembers, depth - 1))
