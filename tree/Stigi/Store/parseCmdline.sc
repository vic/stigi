// prompt: parse NUL-separated Linux process cmdline bytes into load, emit, and mode options.
pub def Stigi.Store.parseCmdline(cmdCap: U64, cmdBytes: Array[U8](cmdCap), cmdLen: U64) <lessOrEqual(cmdLen, cmdCap), below(cmdCap, 0x100000000)>: Stigi.Store.CliOptions =
  var loadStart: U64 = 0
  var loadEnd: U64 = 0
  var emitStart: U64 = 0
  var emitEnd: U64 = 0
  var checkOnly: Bool = false
  var showTests: Bool = false
  var showLaws: Bool = false
  var expectLoad: Bool = false
  var expectEmit: Bool = false
  var argIndex: U64 = 0
  var cursor: U64 = 0
  while cursor < cmdLen && argIndex < cmdLen do
    @<lessOrEqual(cursor, cmdLen), lessOrEqual(argIndex, cmdLen), decreases(cmdLen - argIndex)>
    val argEnd: U64 = Stigi.Store.findByteInRange(cmdCap, cmdBytes, cursor, cmdLen, 0)
    if argIndex > 0 then
      if expectLoad then
        loadStart = cursor
        loadEnd = argEnd
        expectLoad = false
      else if expectEmit then
        emitStart = cursor
        emitEnd = argEnd
        expectEmit = false
      else if Stigi.Parse.sliceEquals(cmdCap, cmdBytes, cursor, argEnd, 6, "--load") then
        expectLoad = true
      else if Stigi.Parse.sliceEquals(cmdCap, cmdBytes, cursor, argEnd, 6, "--emit") then
        expectEmit = true
      else if Stigi.Parse.sliceEquals(cmdCap, cmdBytes, cursor, argEnd, 7, "--check") then
        checkOnly = true
      else if Stigi.Parse.sliceEquals(cmdCap, cmdBytes, cursor, argEnd, 7, "--tests") then
        showTests = true
      else if Stigi.Parse.sliceEquals(cmdCap, cmdBytes, cursor, argEnd, 6, "--laws") then
        showLaws = true
      else
        ()
    else
      ()
    argIndex = argIndex + 1
    cursor = if argEnd < cmdLen then argEnd + 1 else cmdLen
  CliArgs(loadStart, loadEnd, emitStart, emitEnd, checkOnly, showTests, showLaws)
