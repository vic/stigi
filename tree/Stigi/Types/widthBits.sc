// prompt: map a closed integer width to its exact bit count.
pub def Stigi.Types.widthBits(width: Stigi.Types.IntWidth): U64 =
  width match
    case WidthEight => 8
    case WidthThirtyTwo => 32
    case WidthSixtyFour => 64
