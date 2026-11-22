// prompt: count non-end tokens in a source buffer with linear fuel termination.
pub def Stigi.Lex.countTokens(length: U64, source: Array[U8](length)) <below(length, 0x100000000)>: U64 <lessOrEqual(result, length)> =
  var cursor: U64 = 0
  var stepIndex: U64 = 0
  var count: U64 = 0
  var failed: Bool = false
  while cursor < length && stepIndex < length && !failed do
    @<lessOrEqual(cursor, length), lessOrEqual(stepIndex, length), lessOrEqual(count, stepIndex), decreases(length - stepIndex)>
    val outcome: Stigi.Lex.ScanOutcome = Stigi.Lex.nextToken(length, source, cursor)
    outcome match
      case ScanToken(kind, _, endOffset) =>
        kind match
          case TokenEnd =>
            cursor = length
            stepIndex = stepIndex + 1
          case TokenNewline =>
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenIdent =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenUpperIdent =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenInt =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenByte =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenText =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenComment =>
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenPrompt =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
          case TokenPunct =>
            count = count + 1
            if endOffset > cursor && endOffset <= length then
              cursor = endOffset
            else
              cursor = length
            stepIndex = stepIndex + 1
      case ScanError(_, _) =>
        failed = true
        stepIndex = stepIndex + 1
  count
