// prompt: emit a Stigi.Test.expect(actual, expected, msg) check comparing scalars or enums structurally.
def Stigi.Emit.emitTestExpect {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), callPos: U64, stmtEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(callPos, stmtEnd), lessOrEqual(stmtEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
    val Filled(argBegs) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val Filled(argFins) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val argCount: U64 = Stigi.Emit.splitArgs(tokCap, toks, callPos, stmtEnd, argBegs, argFins)
    val argZeroBeg: U64 = argBegs(0)
    val argZeroFin: U64 = argFins(0)
    val argOneBeg: U64 = argBegs(1)
    val argOneFin: U64 = argFins(1)
    if argCount >= 2 && argZeroBeg <= argZeroFin && argZeroFin <= stmtEnd && argOneBeg <= argOneFin && argOneFin <= stmtEnd && monoFin <= srcCap then
      val argInf: Stigi.Emit.InferredType = Stigi.Emit.inferType(srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, localCap, locals, localCount, monoBeg, monoFin)
      val isEnumTest: Bool = argInf match
        case InferEnum(_, _, _) => true
        case InferBool => false
        case InferWord => false
        case InferWordShort => false
        case InferByte => false
        case InferSignedLong => false
        case InferSignedWord => false
        case InferNamed(_, _) => false
        case InferUnknown => false
      if isEnumTest then
        val enumBeg: U64 = argInf match
          case InferEnum(_, begPos, _) => begPos
          case InferBool => 0
          case InferWord => 0
          case InferWordShort => 0
          case InferByte => 0
          case InferSignedLong => 0
          case InferSignedWord => 0
          case InferNamed(_, _) => 0
          case InferUnknown => 0
        val enumFin: U64 = argInf match
          case InferEnum(_, _, finPos) => finPos
          case InferBool => 0
          case InferWord => 0
          case InferWordShort => 0
          case InferByte => 0
          case InferSignedLong => 0
          case InferSignedWord => 0
          case InferNamed(_, _) => 0
          case InferUnknown => 0
        if enumBeg <= enumFin && enumFin <= srcCap then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "  if (!")
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "_eq(")
          Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
          Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argOneBeg, argOneFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 12, ")) return 1;")
        else ()
      else
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "  if ((")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, ") != (")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argOneBeg, argOneFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 12, ")) return 1;")
      target match
        case TargetReturn => Stigi.Emit.writeLine{writer}(outCap, outBuf, 11, "  return 0;")
        case TargetAssign(tempNum) =>
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "  _e")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tempNum)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 5, " = 0;")
        case TargetDiscard => ()
    else ()
  else ()
