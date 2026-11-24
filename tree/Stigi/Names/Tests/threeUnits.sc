// prompt: build the sample of three units over the pool text A.dStigi.hA.n, A.d a def, Stigi.h a handler and A.n a negtest, with the edge from the first to the second unit and one more edge from the given unit to the given target.
pub def Stigi.Names.Tests.threeUnits {region: Stigi.Region, Stigi.Mut} (lastFrom: U64, lastTo: U64) <below(lastFrom, Stigi.Limit.maxUnits), below(lastTo, Stigi.Limit.maxUnits)>: Stigi.Names.Tests.Sample =
  val Filled(pool) = Array.tryFill[U8]{region}(Stigi.Limit.maxIndexBytes, 0) else
    case FillFailed => SampleUnavailable
  val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
    case FillFailed => SampleUnavailable
  val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{region}(Stigi.Names.tableSlots, Vacant) else
    case FillFailed => SampleUnavailable
  val Filled(sources) = Array.tryFill[U64]{region}(Stigi.Names.maxEdges, 0) else
    case FillFailed => SampleUnavailable
  val Filled(targets) = Array.tryFill[U64]{region}(Stigi.Names.maxEdges, 0) else
    case FillFailed => SampleUnavailable
  val Filled(stamps) = Array.tryFill[U64]{region}(Stigi.Limit.maxUnits, 0) else
    case FillFailed => SampleUnavailable
  val copied: U64 = Stigi.Array.copy[U8](13, "A.dStigi.hA.n", 0, 13, Stigi.Limit.maxIndexBytes, pool, 0)
  val empty: Stigi.Names.Image = Image(pool, entries, 0, slots)
  val EdgeKept(firstEdges) = Stigi.Names.addEdge(Edges(sources, targets, 0, stamps), 0, 1) else
    case EdgesFull => SampleUnavailable
  val EdgeKept(edges) = Stigi.Names.addEdge(firstEdges, lastFrom, lastTo) else
    case EdgesFull => SampleUnavailable
  if copied == 13 then
    Stigi.Names.addEntry(empty, 0, 3, Shaped(DefKind, false)) match
      case Added(one) => Stigi.Names.addEntry(one, 3, 10, Shaped(HandlerKind, false)) match
        case Added(two) => Stigi.Names.addEntry(two, 10, 13, Shaped(NegTestKind, false)) match
          case Added(three) => Sampled(three, edges)
          case ImageFull | UnitUnshaped => SampleUnavailable
        case ImageFull | UnitUnshaped => SampleUnavailable
      case ImageFull | UnitUnshaped => SampleUnavailable
  else SampleUnavailable
