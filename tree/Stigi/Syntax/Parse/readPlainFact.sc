// prompt: read one fact without a denial: an optional negation and evidence name, then an atom, an arithmetic fact or a comparison.
def Stigi.Syntax.Parse.readPlainFact {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    val negated: Bool = Stigi.Syntax.Parse.isNot(tables, token)
    val first: U64 = if negated then token + 1 else token
    Stigi.Syntax.Parse.factEvidence(tables, Progress(first, nodes, lists, pending)) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(evidence, afterEvidence) =>
        val Progress(head, evidenceNodes, evidenceLists, evidencePending) = afterEvidence
        val last: U64 = Stigi.Syntax.Parse.nameLast(tables, head, Stigi.Limit.maxUnitBytes)
        if Stigi.Syntax.Parse.isWord(tables, head) && last < Stigi.Limit.maxUnitBytes && Stigi.Syntax.Parse.kindAt(tables, last + 1) is TokLeftParen then
          Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readTerms(tables, Progress(last + 2, evidenceNodes, evidenceLists, evidencePending), Parens)) match
            case ListFailed(fault, at) => Failed(fault, at)
            case Listed(args, after) =>
              Stigi.Syntax.Parse.claimOver(after, evidence, evidence, evidence) match
                case Unclaimed(fault) => Failed(fault, token)
                case Claimed(at, named, _, _) => Stigi.Syntax.Parse.place(tables, at, FactAtom(at, negated, named, Stigi.Syntax.Parse.spanText(tables, head, last), args), after)
        else if Stigi.Syntax.Parse.isWord(tables, head) && head < Stigi.Limit.maxUnitBytes && Stigi.Syntax.Parse.kindAt(tables, head + 1) is TokAssign then
          Stigi.Syntax.Parse.readSides(tables, Progress(head + 2, evidenceNodes, evidenceLists, evidencePending)) match
            case OperandsFailed(fault, at) => Failed(fault, at)
            case Operands(left, operator, right, after) =>
              Stigi.Syntax.Parse.claimOver(after, evidence, left, right) match
                case Unclaimed(fault) => Failed(fault, token)
                case Claimed(at, named, leftSide, rightSide) => Stigi.Syntax.Parse.place(tables, at, FactArith(at, negated, named, Stigi.Syntax.Parse.spanText(tables, head, head), leftSide, operator, rightSide), after)
        else
          Stigi.Syntax.Parse.readSides(tables, afterEvidence) match
            case OperandsFailed(fault, at) => Failed(fault, at)
            case Operands(left, operator, right, after) =>
              Stigi.Syntax.Parse.claimOver(after, evidence, left, right) match
                case Unclaimed(fault) => Failed(fault, token)
                case Claimed(at, named, leftSide, rightSide) => Stigi.Syntax.Parse.place(tables, at, FactCompare(at, negated, named, leftSide, operator, rightSide), after)
