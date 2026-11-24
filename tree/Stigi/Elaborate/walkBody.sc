// prompt: walk one body from its syntax node with the work stack, emitting core nodes in postorder, and give back the core index of the body.
def Stigi.Elaborate.walkBody {Stigi.Mut} (tables: Stigi.Syntax.Tables, bodySyn: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch) <below(bodySyn, Stigi.Limit.maxUnitNodes)>: Stigi.Elaborate.LowerOutcome =
  Stigi.Elaborate.pushWork(scratch, Enter(bodySyn)) match
    case PushFull => LowerOverflow
    case PushDone(seeded) =>
      var current: Stigi.Core.Tables = core
      var stack: Stigi.Elaborate.Scratch = seeded
      var state: Stigi.Elaborate.WalkState = Walking
      while state is Walking do
        stack match
          case WorkScratch(capacity, work, top, mapped, patRes) =>
            if top > 0 then
              val popTop: U64 = top - 1
              val popped: Stigi.Elaborate.Scratch = WorkScratch(capacity, work, popTop, mapped, patRes)
              work(popTop) match
                case Enter(node) =>
                  Stigi.Elaborate.pushChildren(tables, node, popped) match
                    case PushFull => state = WalkOverflow
                    case PushDone(pushed) => stack = pushed
                case Leave(node) =>
                  Stigi.Elaborate.emitCoreNode(tables, node, current, popped) match
                    case EmitFull => state = WalkOverflow
                    case EmitInvalid => state = WalkInvalid
                    case EmitDone(nextCore, nextStack) =>
                      current = nextCore
                      stack = nextStack
            else
              state = WalkFinished
      state match
        case WalkFinished =>
          stack match
            case WorkScratch(capacity, _, _, mapped, _) =>
              if bodySyn < capacity then Lowered(mapped(bodySyn), current, stack) else LowerInvalid
        case WalkOverflow => LowerOverflow
        case WalkInvalid => LowerInvalid
        case Walking => LowerInvalid
