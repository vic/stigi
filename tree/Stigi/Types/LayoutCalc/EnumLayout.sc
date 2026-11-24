// prompt: represent the computed size, alignment, tag placement, and nullable-pointer flag of an enum.
pub enum Stigi.Types.LayoutCalc.EnumLayout:
  case EnumLayoutInfo(byteSize: U64, alignBytes: U64, tagSize: U64, tagOffset: U64, isNullablePtr: Bool, tagInTail: Bool)
  case EnumLayoutOverflow
