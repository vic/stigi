// prompt: write the C11 type name corresponding to a closed Stigi.Emit.InferredType.
def Stigi.Emit.emitInferredCType {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, inferred: Stigi.Emit.InferredType) <below(0, outCap), lessOrEqual(symCount, symCap)>: Unit =
  inferred match
    case InferBool => Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "bool")
    case InferWord => Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint64_t")
    case InferWordShort => Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint32_t")
    case InferByte => Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "uint8_t")
    case InferSignedLong => Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "int64_t")
    case InferSignedWord => Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "int32_t")
    case InferEnum(_, enumBeg, enumFin) =>
      if enumFin <= srcCap then Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin) else Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint64_t")
    case InferNamed(nameBeg, nameFin) =>
      if nameFin <= srcCap then Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, nameBeg, nameFin, 0, 0, 0, 0) else Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint64_t")
    case InferUnknown => Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint64_t")
