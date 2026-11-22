// prompt: enum memory layout computed for C11 emission including 1-word and tail-padding forms.
pub enum Stigi.Types.EnumLayout:
  case LayoutNullablePtr(nullCaseIndex: U64, byteSize: U64, byteAlign: U64)
  case LayoutTailTag(byteSize: U64, byteAlign: U64, tagOffset: U64)
  case LayoutTagged(byteSize: U64, byteAlign: U64, payloadOffset: U64)
