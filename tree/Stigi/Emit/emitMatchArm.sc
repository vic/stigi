// prompt: emit one match case arm with pattern conditions, payload bindings, optional guard, and arm body.
def Stigi.Emit.emitMatchArm {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), patStart: U64, patEnd: U64, guardPos: U64, arrowPos: U64, armBodyStart: U64, armBodyEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, scrutTemps: Array[U64](4), paramBegs: Array[U64](4), paramFins: Array[U64](4), flagNum: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(patStart, patEnd), lessOrEqual(patEnd, tokCap), lessOrEqual(arrowPos, tokCap), lessOrEqual(armBodyStart, armBodyEnd), lessOrEqual(armBodyEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 11, "  if (!_mat")
    Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, flagNum)
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, " && (")
    val Filled(patBegs) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val Filled(patFins) = Array.tryFill[U64]{region}(32, 0) else
      case FillFailed => ()
    val isTuplePat: Bool = if patStart < patEnd then
      val Token(patLead, _, _, _, _) = toks(patStart)
      patLead == TokLeftParen
    else false
    val patCount: U64 = if isTuplePat then Stigi.Emit.splitArgs(tokCap, toks, patStart, patEnd, patBegs, patFins) else 1
    if !isTuplePat then
      patBegs(0) := patStart
      patFins(0) := patEnd
    else ()
    var patIdx: U64 = 0
    while patIdx < patCount && patIdx < 4 do
      @<lessOrEqual(patIdx, patCount), lessOrEqual(patIdx, 4)>
      if patIdx > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " && ") else ()
      val patSegBeg: U64 = patBegs(patIdx)
      val patSegFin: U64 = patFins(patIdx)
      val parSegBeg: U64 = paramBegs(patIdx)
      val parSegFin: U64 = paramFins(patIdx)
      if patSegBeg <= patSegFin && patSegFin <= tokCap && parSegBeg <= parSegFin && parSegFin <= srcCap then
        Stigi.Emit.emitPatCond{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, patSegBeg, patSegFin, symCap, syms, symCount, scrutTemps(patIdx), parSegBeg, parSegFin)
      else Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true")
      patIdx = patIdx + 1
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 4, ")) {")
    var armLocals: U64 = localCount
    var bindIdx: U64 = 0
    while bindIdx < patCount && bindIdx < 4 do
      @<lessOrEqual(bindIdx, patCount), lessOrEqual(bindIdx, 4), lessOrEqual(armLocals, localCap)>
      val patSegBeg: U64 = patBegs(bindIdx)
      val patSegFin: U64 = patFins(bindIdx)
      val parSegBeg: U64 = paramBegs(bindIdx)
      val parSegFin: U64 = paramFins(bindIdx)
      if patSegBeg <= patSegFin && patSegFin <= tokCap && parSegBeg <= parSegFin && parSegFin <= srcCap then
        armLocals = Stigi.Emit.emitPatBinds{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, patSegBeg, patSegFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, armLocals, scrutTemps(bindIdx), parSegBeg, parSegFin)
      else ()
      bindIdx = bindIdx + 1
    if guardPos < arrowPos then
      val guardStart: U64 = guardPos + 1
      if guardStart < arrowPos then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "    if (")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, guardStart, arrowPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, armLocals, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "    _mat")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, flagNum)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 8, " = true;")
        Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, armBodyStart, armBodyEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, armLocals, meta, target, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 5, "    }")
      else ()
    else
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "    _mat")
      Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, flagNum)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 8, " = true;")
      Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, armBodyStart, armBodyEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, armLocals, meta, target, nextFuel)
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "  }")
  else ()
