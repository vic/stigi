// prompt: represent one directed dependency edge between two type declarations.
pub enum Stigi.Types.Cycles.TypeEdge:
  case DepEdge(fromType: U64, toType: U64, edgeKind: Stigi.Types.Cycles.EdgeKind)
