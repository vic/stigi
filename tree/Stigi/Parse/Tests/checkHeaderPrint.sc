// prompt: verify canonical header printing via stack-allocated BufferWriter handler.
def Stigi.Parse.Tests.checkHeaderPrint {region: Stigi.Region, Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, header: Stigi.Ast.UnitHeader) <lessOrEqual(srcLen, srcCap)>: Bool =
  handle writer = Stigi.Emit.BufferWriter(128)
  val Filled(outBuf) = Array.tryFill[U8]{region}(128, 0) else
    case FillFailed => false
  val printed: Bool = Stigi.Print.printHeader{writer}(128, outBuf, srcCap, src, header)
  val printedLen: U64 = writer.writtenCount()
  val tempZero: U64 = writer.nextTemp()
  printed && printedLen == 18 && tempZero == 0 && srcLen >= 17 && Stigi.Lex.sliceEquals(128, outBuf, 0, 18, 18, "pub def Demo.calc\n")
