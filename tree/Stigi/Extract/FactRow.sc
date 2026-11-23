// prompt: single-case SSA fact row pairing relation kind with three value or scope columns.
pub enum Stigi.Extract.FactRow:
  case FactRow(kind: Stigi.Extract.FactKind, colZero: U64, colOne: U64, colTwo: U64)
