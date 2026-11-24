// prompt: verify that deep facts, VarStmt invariants, local extern defs, calls with capArgs, handle stmts, and non-recursive declarations round-trip at max depth.
val Stigi.Syntax.Print.Tests.metaDepthSuite {Stigi.Test}: Stigi.Test.Result =
  val factsPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDeepFact, 61, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDeepFact, 62, false)
  val invariantPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestVarInvariant, 60, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestVarInvariant, 61, false)
  val externPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestLocalExtern, 61, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestLocalExtern, 62, false)
  val callPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDeepCall, 59, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDeepCall, 60, false)
  val handlePassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDeepHandle, 60, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDeepHandle, 61, false)
  val typeDeclDeepPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestTypeDecl, 63, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestTypeDecl, 64, false)
  val defReturnDeepPassed: Bool = Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDefReturn, 63, true) && Stigi.Syntax.Print.Tests.checkNest{Stigi.Region}(NestDefReturn, 64, false)
  val declsPassed: Bool =
    val bufferCap: U64 = 512
    val Filled(sourceBuffer) = Array.tryFill[U8]{Stigi.Region}(bufferCap, 0) else
      case FillFailed => false
    val enumSrc: Array[U8](80) = "// prompt: e.\nenum Demo.E [T: Type]:\n  case A(x: T) <p(0)>\n  case B(y: A[B[C]])\n"
    val typeSrc: Array[U8](69) = "// prompt: t.\ntype Demo.T [A: Type] (n: U64) {r: R} = A[B[C]] <p(0)>\n"
    val ruleSrc: Array[U8](56) = "// prompt: r.\nrule def Demo.r: p(0) = not (q(0)), 0 < 1\n"
    val enumPassed: Bool = Stigi.Syntax.Print.Tests.checkBufferRoundTrip{Stigi.Region}(bufferCap, sourceBuffer, Stigi.Array.copy[U8](80, enumSrc, 0, 80, bufferCap, sourceBuffer, 0), true)
    val typePassed: Bool = Stigi.Syntax.Print.Tests.checkBufferRoundTrip{Stigi.Region}(bufferCap, sourceBuffer, Stigi.Array.copy[U8](69, typeSrc, 0, 69, bufferCap, sourceBuffer, 0), true)
    val rulePassed: Bool = Stigi.Syntax.Print.Tests.checkBufferRoundTrip{Stigi.Region}(bufferCap, sourceBuffer, Stigi.Array.copy[U8](56, ruleSrc, 0, 56, bufferCap, sourceBuffer, 0), true)
    enumPassed && typePassed && rulePassed && typeDeclDeepPassed && defReturnDeepPassed
  Stigi.Test.expect(factsPassed && invariantPassed && externPassed && callPassed && handlePassed && declsPassed, true, "deep facts, var invariants, local extern defs, capArgs, and decl parts must round-trip at max bounds")
