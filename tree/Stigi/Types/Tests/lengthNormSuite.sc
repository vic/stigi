// prompt: verify canonical commutativity, oriented termOffset, known-binder subtraction, and nonlinear/negative rejections.
val Stigi.Types.Tests.lengthNormSuite {Stigi.Test}: Stigi.Test.Result =
  val srcVarPlusOne: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n + 1\n"
  val srcOnePlusVar: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  1 + n\n"
  val srcTwoFirst: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n + k\n"
  val srcTwoSecond: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  k + n\n"
  val srcSubKnown: Array[U8](88) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n + 2 - 1\n"
  val srcPlusFive: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n + 5\n"
  val srcPlusTwo: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n + 2\n"
  val srcMul: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  a * b\n"
  val srcDiv: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n / k\n"
  val srcNegSub: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  n - 1\n"
  val srcVarSub: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  2 - n\n"
  val srcMutUse: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  v + 1\n"
  val srcDivZero: Array[U8](84) = "// prompt: len.\ndef Demo.len(n: U64, k: U64, a: U64, b: U64, v: U64): U64 =\n  1 / 0\n"
  val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
    case AllocationFailed => Stigi.Test.expect(false, true, "syntax table alloc")
  val ScratchReady(scratch) = Stigi.Elaborate.allocateScratch{Stigi.Region}(128) else
    case ScratchUnavailable => Stigi.Test.expect(false, true, "scratch alloc")
  val Filled(pool) = Array.tryFill[Stigi.Types.Linear.Term]{Stigi.Region}(64, LinearTerm(0, WideInt(0, 0))) else
    case FillFailed => Stigi.Test.expect(false, true, "pool alloc")
  val Filled(binders) = Array.tryFill[Stigi.Types.BinderKind]{Stigi.Region}(8, ImmutableBinder) else
    case FillFailed => Stigi.Test.expect(false, true, "binders alloc")
  val Filled(globals) = Array.tryFill[Stigi.Types.BinderKind]{Stigi.Region}(4, ImmutableBinder) else
    case FillFailed => Stigi.Test.expect(false, true, "globals alloc")
  val Filled(nodeStack) = Array.tryFill[U64]{Stigi.Region}(64, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "node stack alloc")
  val Filled(stepStack) = Array.tryFill[U64]{Stigi.Region}(64, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "step stack alloc")
  val Filled(formStack) = Array.tryFill[Stigi.Types.Linear.Form]{Stigi.Region}(64, LinearForm(WideInt(0, 0), 0, 0)) else
    case FillFailed => Stigi.Test.expect(false, true, "form stack alloc")
  binders(4) := MutableBinder
  val TableReady(table) = Stigi.Types.allocateTable{Stigi.Region}(8, 16, 8, 8, 8) else
    case TableUnavailable => Stigi.Test.expect(false, true, "type table alloc")
  val LengthValue(formLeft, poolOne) = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, 0, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcVarPlusOne) else
    case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "n + 1")
  if poolOne > 64 then Stigi.Test.expect(false, true, "pool bound one") else
    val LengthValue(formRight, poolTwo) = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolOne, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcOnePlusVar) else
      case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "1 + n")
    if poolTwo > 64 then Stigi.Test.expect(false, true, "pool bound two") else
      val LengthValue(formSub, poolThree) = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolTwo, 8, binders, 4, globals, nodeStack, stepStack, formStack), 88, srcSubKnown) else
        case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "n + 2 - 1")
      if poolThree > 64 then Stigi.Test.expect(false, true, "pool bound three") else
        val LengthValue(formFive, poolFour) = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolThree, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcPlusFive) else
          case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "n + 5")
        if poolFour > 64 then Stigi.Test.expect(false, true, "pool bound four") else
          val LengthValue(formTwo, poolFive) = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFour, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcPlusTwo) else
            case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "n + 2")
          if poolFive > 64 then Stigi.Test.expect(false, true, "pool bound five") else
            val LengthValue(formTwoFirst, poolSix) = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcTwoFirst) else
              case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "n + k")
            val LengthValue(formTwoSecond, poolSeven) = if poolSix <= 64 then Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolSix, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcTwoSecond) else LengthPoolFull else
              case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => Stigi.Test.expect(false, true, "k + n")
            val commSame: Bool = poolSeven <= 64 && Stigi.Types.sameLength(64, pool, formLeft, formRight) && Stigi.Types.sameLength(64, pool, formLeft, formSub) && Stigi.Types.sameLength(64, pool, formTwoFirst, formTwoSecond)
            val offsetValid: Bool = Stigi.Types.termOffsetOf(64, pool, formFive, formTwo) is TermOffsetPlus(3) && Stigi.Types.termOffsetOf(64, pool, formTwo, formFive) is WithoutTermOffset && Stigi.Types.termOffsetOf(64, pool, formLeft, formRight) is WithoutTermOffset
            val boundPool: U64 = if poolSeven <= 64 then poolSeven else 64
            binders(0) := KnownBinder(LinearForm(WideInt(5, 0), 0, 0))
            val knownSubValid: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, boundPool, 8, binders, 4, globals, nodeStack, stepStack, formStack), 88, srcSubKnown) match
              case LengthValue(knownForm, _) => Stigi.Types.sameLength(64, pool, knownForm, LinearForm(WideInt(6, 0), 0, 0))
              case LengthNotLinear | LengthOverflow | LengthDivZero | LengthPoolFull | LengthNotUnsigned => false
            binders(0) := ImmutableBinder
            val rejMul: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcMul) is LengthNotLinear
            val rejDiv: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcDiv) is LengthNotLinear
            val rejNeg: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcNegSub) is LengthNotUnsigned
            val rejVar: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcVarSub) is LengthNotUnsigned
            val rejMut: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcMutUse) is LengthNotLinear
            val rejZero: Bool = Stigi.Types.Tests.parseNormExpr{Stigi.Region}(tables, scratch, LengthSpace(64, pool, poolFive, 8, binders, 4, globals, nodeStack, stepStack, formStack), 84, srcDivZero) is LengthDivZero
            val InternedAt(withByte, byteAt) = Stigi.Types.intern(table, Primitive(UnsignedByte)) else
              case TypeTableFull => Stigi.Test.expect(false, true, "intern U8")
            val StagedRows(withArg, argRows) = Stigi.Types.stageArgument(withByte, byteAt) else
              case StageFull | StageNotUnsigned | StageBadBinder => Stigi.Test.expect(false, true, "stage arg")
            val StagedRows(withLeftLen, leftRows) = Stigi.Types.stageLength(withArg, 64, pool, formLeft) else
              case StageFull | StageNotUnsigned | StageBadBinder => Stigi.Test.expect(false, true, "stage left")
            val InternedAt(afterLeft, leftTypeAt) = Stigi.Types.intern(withLeftLen, Named(ExternalRef(4, 0), argRows, leftRows)) else
              case TypeTableFull => Stigi.Test.expect(false, true, "intern left")
            val StagedRows(withRightLen, rightRows) = Stigi.Types.stageLength(afterLeft, 64, pool, formRight) else
              case StageFull | StageNotUnsigned | StageBadBinder => Stigi.Test.expect(false, true, "stage right")
            val InternedAt(_, rightTypeAt) = Stigi.Types.intern(withRightLen, Named(ExternalRef(4, 0), argRows, rightRows)) else
              case TypeTableFull => Stigi.Test.expect(false, true, "intern right")
            val internSame: Bool = leftTypeAt == rightTypeAt
            Stigi.Test.expect(commSame && offsetValid && knownSubValid && rejMul && rejDiv && rejNeg && rejVar && rejMut && rejZero && internSame, true, "lengthOf must normalize linear terms canonically, orient termOffset, and reject non-linear/negative forms")
