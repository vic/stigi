// prompt: verify the printer helpers: writes, the shape of an absent slot and every parenthesizing rule.
val Stigi.Syntax.Print.Tests.helperSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val written: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.putUnary(tables, Stigi.Syntax.Print.putBinary(tables, Stigi.Syntax.Print.putNewline(tables, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putText(tables, Pen(0), Text(0, 0)), 2, "U8"), 2), Plus), Negate)
    val Tables(_, _, _, _, _, output) = tables
    val writesValid: Bool = written is Pen(9) && output(0) == 'U' && output(2) == '\n' && output(3) == ' ' && output(6) == '+' && output(8) == '-'
    val shapesValid: Bool = Stigi.Syntax.Print.shapeOf(tables, 0) is ShapePlain && Stigi.Syntax.Print.isAbsent(tables, 0) && Stigi.Syntax.Print.hasItems(Span(0, 1)) && !Stigi.Syntax.Print.hasItems(Span(1, 1))
    val parensValid: Bool = Stigi.Syntax.Print.atomParens(ShapeStatement) && !Stigi.Syntax.Print.atomParens(ShapeIs) && Stigi.Syntax.Print.termParens(ShapeControl) && Stigi.Syntax.Print.scrutineeParens(ShapeControl) && Stigi.Syntax.Print.subjectParens(ShapeBinary(3)) && !Stigi.Syntax.Print.subjectParens(ShapeBinary(5)) && Stigi.Syntax.Print.sideParens(ShapeBinary(5), 5, true) && !Stigi.Syntax.Print.sideParens(ShapeBinary(5), 5, false) && Stigi.Syntax.Print.operandParens(ShapeUnary(true), true) && !Stigi.Syntax.Print.operandParens(ShapeUnary(false), true)
    writesValid && shapesValid && parensValid
  Stigi.Test.expect(passed, true, "printer helpers write bytes and parenthesize by the seed printer rules")
