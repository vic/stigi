// prompt: step over a parenthesized run to its closing parenthesis, counting its top-level arguments.
def Stigi.Parse.scanArgs {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, state: Stigi.Parse.ArgState, remaining: U64) <lessOrEqual(tokCount, tokCap), decreases(remaining)>: Stigi.Parse.ArgScan =
  val ArgState(depth, firstArg, argCount) = state
  val pos: U64 = cursor.curPos()
  if remaining < 1 then ArgsOpen else
    Stigi.Parse.nestingOf(Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)) match
      case Ends => ArgsOpen
      case Opens =>
        cursor.stepForward()
        if depth < Stigi.Limit.maxTokens then Stigi.Parse.scanArgs{cursor}(tokCap, toks, tokCount, ArgState(depth + 1, firstArg, argCount), remaining - 1) else ArgsOpen
      case Closes =>
        cursor.stepForward()
        if depth < 2 then ArgsClosed(firstArg, argCount) else Stigi.Parse.scanArgs{cursor}(tokCap, toks, tokCount, ArgState(depth - 1, firstArg, argCount), remaining - 1)
      case Separates =>
        cursor.stepForward()
        val nextCount: U64 = if depth == 1 && argCount < Stigi.Limit.maxTokens then argCount + 1 else argCount
        Stigi.Parse.scanArgs{cursor}(tokCap, toks, tokCount, ArgState(depth, firstArg, nextCount), remaining - 1)
      case Inside =>
        cursor.stepForward()
        val nextState: Stigi.Parse.ArgState = if depth == 1 && argCount == 0 then ArgState(depth, pos, 1) else state
        Stigi.Parse.scanArgs{cursor}(tokCap, toks, tokCount, nextState, remaining - 1)
