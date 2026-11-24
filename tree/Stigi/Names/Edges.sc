// prompt: the dependency edges found so far: pairs of unit entries, their count, and a stamp per target that drops repeats for one source.
pub enum Stigi.Names.Edges:
  case Edges(sources: Array[U64](Stigi.Names.maxEdges), targets: Array[U64](Stigi.Names.maxEdges), count: U64, stamps: Array[U64](Stigi.Limit.maxUnits)) <lessOrEqual(count, Stigi.Names.maxEdges)>
