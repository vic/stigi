// prompt: write one typeWidth fact row into a bounded buffer at a position, giving the new end.
def Stigi.Store.putTypeWidthRow {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, primitive: Stigi.Core.Primitive, width: Stigi.Types.IntWidth) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val named: U64 = Stigi.Store.putPrimitiveName(bufCap, buf, at, primitive)
  val tabbed: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, bufCap, buf, named)
  val bitsEnd: U64 = Stigi.Store.putUnsigned{region}(bufCap, buf, tabbed, Stigi.Types.widthBits(width))
  Stigi.Array.copy[U8](1, "\n", 0, 1, bufCap, buf, bitsEnd)
