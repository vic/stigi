// prompt: verify Stigi.Emit declaration indexing, enum topological emission, capabilities, and prototypes.
val Stigi.Emit.Tests.emitterSuite {Stigi.Test}: Stigi.Test.Result =
  handle writer = Stigi.Emit.BufferWriter(8192)
  val sampleSrc: Array[U8](87) = "// prompt: demo enum.\nenum Demo.Pair:\n  case Pair(left: U64, right: Bool)\n  case Empty\n"
  val Filled(toks) = Array.tryFill[Stigi.Lex.Token]{Stigi.Region}(64, Token(TokEnd, 0, 0, 0, 0)) else
    case FillFailed => Stigi.Test.expect(false, true, "token buf")
  val Filled(syms) = Array.tryFill[Stigi.Emit.SymEntry]{Stigi.Region}(16, SymEntry(SymFunc, 0, 0, 0, 0, 0, 0, 0, 0, 0)) else
    case FillFailed => Stigi.Test.expect(false, true, "sym buf")
  val Filled(fields) = Array.tryFill[Stigi.Emit.FieldInfo]{Stigi.Region}(16, FieldInfo(FieldParam, 0, 0, 0, 0, 0, 0)) else
    case FillFailed => Stigi.Test.expect(false, true, "field buf")
  val Filled(outBuf) = Array.tryFill[U8]{Stigi.Region}(8192, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "emit buf")
  val LexDone(tokCount) = Stigi.Lex.scanAll(87, sampleSrc, 87, 64, toks) else
    case LexOverflow => Stigi.Test.expect(false, true, "lex overflow")
    case LexSyntaxError(_, _) => Stigi.Test.expect(false, true, "lex syntax")
  if tokCount <= 64 then
    val IndexCounts(symCount, fieldCount, _) = Stigi.Emit.indexUnit(87, sampleSrc, 64, toks, 0, tokCount, 16, syms, 0, 16, fields, 0)
    if symCount <= 16 && fieldCount <= 16 then
      Stigi.Emit.emitPreamble{writer}(8192, outBuf)
      Stigi.Emit.emitEnums{Stigi.Region, writer}(8192, outBuf, 87, sampleSrc, 16, syms, symCount, 16, fields, fieldCount)
      Stigi.Emit.emitCapsAndHandlers{Stigi.Region, writer}(8192, outBuf, 87, sampleSrc, 16, syms, symCount, 16, fields, fieldCount)
      Stigi.Emit.emitProtoDispatch{Stigi.Region, writer}(8192, outBuf, 87, sampleSrc, 16, syms, symCount, 16, fields, fieldCount)
      Stigi.Emit.emitPrototypes{writer}(8192, outBuf, 87, sampleSrc, 64, toks, 16, syms, symCount, 16, fields, fieldCount)
      Stigi.Emit.emitMain{writer}(8192, outBuf, 87, sampleSrc, 16, syms, symCount)
      val written: U64 = writer.writtenCount()
      val tgtVal: Stigi.Emit.TargetKind = TargetReturn
      val tgtCheck: Bool = tgtVal match
        case TargetReturn => true
        case TargetAssign(_) => false
        case TargetDiscard => false
      Stigi.Test.expect(symCount == 3 && fieldCount == 2 && written > 1000 && !writer.didOverflow() && tgtCheck, true, "emitter must index enum and emit C11 preamble, enum, and main cleanly")
    else Stigi.Test.expect(false, true, "counts bounded")
  else Stigi.Test.expect(false, true, "tokCount bounded")
