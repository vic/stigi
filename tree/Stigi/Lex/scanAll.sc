// prompt: scan a full L0 source buffer into a single contiguous typed Token array.
pub def Stigi.Lex.scanAll {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, tokCap: Stigi.TokenCap, outToks: Array[Stigi.Lex.Token](tokCap)) <lessOrEqual(srcLen, srcCap)>: Stigi.Lex.LexResult =
  Stigi.Lex.scanFrom(srcCap, src, srcLen, tokCap, outToks, Cursor(0, 1, 0, 0), srcLen)
