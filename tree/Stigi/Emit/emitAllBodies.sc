// prompt: emit C11 definitions for all indexed executable symbols by dispatching to emitOneBody and emitGenericBody.
def Stigi.Emit.emitAllBodies {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64) <below(0, outCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>: Unit =
  val Filled(locals) = Array.tryFill[Stigi.Emit.FieldInfo]{region}(256, FieldInfo(FieldLocal, 0, 0, 0, 0, 0, 0)) else
    case FillFailed => ()
  var symIdx: U64 = 0
  while symIdx < symCount do
    @<lessOrEqual(symIdx, symCount)>
    val SymEntry(symKind, _, _, _, _, _, _, _, _, _) = syms(symIdx)
    val isNormalBody: Bool = symKind == SymExternBody || symKind == SymGivenMethod || symKind == SymHandlerMethod || symKind == SymFunc || symKind == SymTest
    if isNormalBody then
      Stigi.Emit.emitOneBody{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, symCap, syms, symCount, fieldCap, fields, fieldCount, locals, symIdx)
    else if symKind == SymGenericFunc then
      Stigi.Emit.emitGenericBody{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, symCap, syms, symCount, fieldCap, fields, fieldCount, locals, symIdx)
    else ()
    symIdx = symIdx + 1
