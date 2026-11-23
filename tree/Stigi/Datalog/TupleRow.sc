// prompt: single-case n-ary Datalog tuple row storing relation tag and three columns.
pub enum Stigi.Datalog.TupleRow:
  case TupleRow(relTag: U64, colZero: U64, colOne: U64, colTwo: U64)
