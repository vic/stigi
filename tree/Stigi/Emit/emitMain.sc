// prompt: emit stigi_run_on_stack, portable stack trampoline, and C main entrypoint.
def Stigi.Emit.emitMain {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64) <lessOrEqual(symCount, symCap)>: Unit =
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 48, "static int stigi_run_on_stack(StigiRtState *s) {")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 10, "  (void)s;")
  var symIdx: U64 = 0
  while symIdx < symCount do
    @<lessOrEqual(symIdx, symCount)>
    val SymEntry(symKind, _, nameBeg, nameFin, _, _, _, _, _, _) = syms(symIdx)
    if symKind == SymTest && nameFin <= srcCap then
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "  if (")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 20, "() != 0) return 100;")
    else ()
    symIdx = symIdx + 1
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 27, "  return (int)Stigi_main();")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 16, "int main(void) {")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 18, "  StigiRtState st;")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 50, "  if (stigi_rt_init(&st) != STIGI_RT_OK) return 1;")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 35, "  int rc = stigi_run_on_stack(&st);")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 25, "  stigi_rt_shutdown(&st);")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 12, "  return rc;")
  Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
