// prompt: emit the S118 guard-paged stack trampoline and main entry invoking all Tests and Stigi_main.
pub def Stigi.Emit.writeRuntimeEntry {Stigi.Mut} (unitCount: U64, order: Array[U64](unitCount), indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), hasMain: Bool, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), below(indexCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLine(30, "#line 1 \"<stigi-runtime-main>\"", outCap, output, writeOffset)
  pos = Stigi.Emit.writeLine(43, "static int stigi_run_on_stack(StigiRt *r) {", outCap, output, pos)
  pos = Stigi.Emit.writeLine(10, "  (void)r;", outCap, output, pos)
  var slotIndex: U64 = 0
  while slotIndex < unitCount do
    @<lessOrEqual(slotIndex, unitCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(unitCount - slotIndex)>
    val unitIdx: U64 = order(slotIndex)
    if unitIdx < unitCount && kindCodes(unitIdx) == 11 then
      val startName: U64 = nameStarts(unitIdx)
      val endName: U64 = nameEnds(unitIdx)
      if startName <= endName && endName <= indexCap then
        pos = Stigi.Emit.writeLiteral(12, "  if (Stigi_", outCap, output, pos)
        var charIdx: U64 = startName
        while charIdx < endName do
          @<lessOrEqual(startName, charIdx), lessOrEqual(charIdx, endName), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(endName - charIdx)>
          val rawByte: U8 = indexBytes(charIdx)
          val mappedByte: U8 = if rawByte == 46 then 95 else rawByte
          pos = Stigi.Emit.writeByte(outCap, output, pos, mappedByte)
          charIdx = charIdx + 1
        pos = Stigi.Emit.writeLine(20, "() != 0) return 100;", outCap, output, pos)
      else
        ()
    else
      ()
    slotIndex = slotIndex + 1
  if hasMain then
    pos = Stigi.Emit.writeLine(27, "  return (int)Stigi_main();", outCap, output, pos)
  else
    pos = Stigi.Emit.writeLine(11, "  return 0;", outCap, output, pos)
  pos = Stigi.Emit.writeLine(1, "}", outCap, output, pos)
  pos = Stigi.Emit.writeLine(45, "#if defined(__x86_64__) && !defined(__FILC__)", outCap, output, pos)
  pos = Stigi.Emit.writeLine(79, "static int stigi_call_on_stack(void *top, int (*fn)(StigiRt *), StigiRt *arg) {", outCap, output, pos)
  pos = Stigi.Emit.writeLine(60, "  uintptr_t sp = (((uintptr_t)top) & ~(uintptr_t)15U) - 16U;", outCap, output, pos)
  pos = Stigi.Emit.writeLine(57, "  int (*f)(StigiRt *) = fn; StigiRt *a = arg; int rc = 0;", outCap, output, pos)
  pos = Stigi.Emit.writeLine(23, "  __asm__ __volatile__(", outCap, output, pos)
  pos = Stigi.Emit.writeLine(79, "    \"movq %%rsp, (%1)\\n\\tmovq %1, %%rsp\\n\\tcall *%2\\n\\tmovq (%%rsp), %%rsp\\n\\t\"", outCap, output, pos)
  pos = Stigi.Emit.writeLine(42, "    : \"=a\"(rc), \"+r\"(sp), \"+S\"(f), \"+D\"(a)", outCap, output, pos)
  pos = Stigi.Emit.writeLine(64, "    : : \"rcx\", \"rdx\", \"r8\", \"r9\", \"r10\", \"r11\", \"memory\", \"cc\");", outCap, output, pos)
  pos = Stigi.Emit.writeLine(12, "  return rc;", outCap, output, pos)
  pos = Stigi.Emit.writeLine(1, "}", outCap, output, pos)
  pos = Stigi.Emit.writeLine(5, "#else", outCap, output, pos)
  pos = Stigi.Emit.writeLine(79, "static int stigi_call_on_stack(void *top, int (*fn)(StigiRt *), StigiRt *arg) {", outCap, output, pos)
  pos = Stigi.Emit.writeLine(28, "  (void)top; return fn(arg);", outCap, output, pos)
  pos = Stigi.Emit.writeLine(1, "}", outCap, output, pos)
  pos = Stigi.Emit.writeLine(6, "#endif", outCap, output, pos)
  pos = Stigi.Emit.writeLine(16, "int main(void) {", outCap, output, pos)
  pos = Stigi.Emit.writeLine(42, "  StigiRt rt; int st = stigi_rt_init(&rt);", outCap, output, pos)
  pos = Stigi.Emit.writeLine(53, "  if (st != 0) { stigi_rt_shutdown(&rt); return st; }", outCap, output, pos)
  pos = Stigi.Emit.writeLine(83, "  int rc = stigi_call_on_stack((char *)rt.sraw + rt.stot, stigi_run_on_stack, &rt);", outCap, output, pos)
  pos = Stigi.Emit.writeLine(36, "  stigi_rt_shutdown(&rt); return rc;", outCap, output, pos)
  Stigi.Emit.writeLine(1, "}", outCap, output, pos)
