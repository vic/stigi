// prompt: build a synthetic unit nesting one construct to the given depth, parse it, and verify exact round-trip printing or TooDeep rejection.
def Stigi.Syntax.Print.Tests.checkNest {region: Stigi.Region, Stigi.Mut} (kind: Stigi.Syntax.Print.Tests.NestKind, depth: U64, expectAccept: Bool) <lessOrEqual(depth, Stigi.Limit.maxUnitBytes)>: Bool =
  val bufferCap: U64 = 4096
  val Filled(sourceBuffer) = Array.tryFill[U8]{region}(bufferCap, 0) else
    case FillFailed => false
  val afterHead: U64 = kind match
    case NestType =>
      val head: Array[U8](32) = "// prompt: d.\ndef Demo.check(x: "
      Stigi.Array.copy[U8](32, head, 0, 32, bufferCap, sourceBuffer, 0)
    case NestTypeDecl =>
      val head: Array[U8](28) = "// prompt: d.\ntype Demo.T = "
      Stigi.Array.copy[U8](28, head, 0, 28, bufferCap, sourceBuffer, 0)
    case NestDefReturn =>
      val head: Array[U8](32) = "// prompt: d.\ndef Demo.check(): "
      Stigi.Array.copy[U8](32, head, 0, 32, bufferCap, sourceBuffer, 0)
    case NestPattern | NestPatOr =>
      val head: Array[U8](59) = "// prompt: d.\ndef Demo.check(x: U64): U64 = x match\n  case "
      Stigi.Array.copy[U8](59, head, 0, 59, bufferCap, sourceBuffer, 0)
    case NestBinary | NestParen | NestIs | NestCall | NestAssert =>
      val head: Array[U8](44) = "// prompt: d.\ndef Demo.check(x: U64): U64 = "
      Stigi.Array.copy[U8](44, head, 0, 44, bufferCap, sourceBuffer, 0)
    case NestIf | NestDeepFact | NestVarInvariant | NestLocalExtern | NestDeepCall | NestDeepHandle =>
      val head: Array[U8](45) = "// prompt: d.\ndef Demo.check(x: Bool): U64 = "
      Stigi.Array.copy[U8](45, head, 0, 45, bufferCap, sourceBuffer, 0)
  val afterOpen: U64 = kind match
    case NestType | NestTypeDecl | NestDefReturn =>
      val fragment: Array[U8](2) = "A["
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterHead, 2, fragment, depth)
    case NestPattern =>
      val fragment: Array[U8](2) = "C("
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterHead, 2, fragment, depth)
    case NestPatOr =>
      val fragment: Array[U8](6) = "C(a | "
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterHead, 6, fragment, depth)
    case NestIf | NestDeepFact | NestVarInvariant | NestLocalExtern | NestDeepCall | NestDeepHandle =>
      val fragment: Array[U8](10) = "if x then "
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterHead, 10, fragment, depth)
    case NestParen =>
      val fragment: Array[U8](5) = "1 + ("
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterHead, 5, fragment, depth)
    case NestBinary =>
      val fragment: Array[U8](4) = "1 + "
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterHead, 4, fragment, depth)
    case NestIs | NestCall | NestAssert =>
      val fragment: Array[U8](1) = "x"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterHead, 1, fragment)
  val afterCenter: U64 = if bufferCap < afterOpen then bufferCap + 1 else kind match
    case NestType | NestTypeDecl | NestDefReturn =>
      val center: Array[U8](1) = "B"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 1, center)
    case NestPattern | NestPatOr =>
      val center: Array[U8](1) = "x"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 1, center)
    case NestBinary =>
      val center: Array[U8](1) = "1"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 1, center)
    case NestParen =>
      val center: Array[U8](5) = "1 + 1"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 5, center)
    case NestIf =>
      val center: Array[U8](1) = "0"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 1, center)
    case NestDeepFact =>
      val center: Array[U8](8) = "y@<p(0)>"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 8, center)
    case NestVarInvariant =>
      val part: Array[U8](21) = "var y@<p(0)>: U64 = 0"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 21, part)
    case NestLocalExtern =>
      val center: Array[U8](49) = "extern \"c\" def helper(a: U64) <below(a, 10)>: U64"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 49, center)
    case NestDeepCall =>
      val center: Array[U8](10) = "f[A]{c}(0)"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 10, center)
    case NestDeepHandle =>
      val center: Array[U8](18) = "handle h = H{c}(0)"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterOpen, 18, center)
    case NestIs | NestCall | NestAssert => afterOpen
  val afterClose: U64 = if bufferCap < afterCenter then bufferCap + 1 else kind match
    case NestType | NestTypeDecl | NestDefReturn =>
      val closer: Array[U8](1) = "]"
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterCenter, 1, closer, depth)
    case NestPattern | NestPatOr | NestParen =>
      val closer: Array[U8](1) = ")"
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterCenter, 1, closer, depth)
    case NestIf | NestDeepFact | NestVarInvariant | NestLocalExtern | NestDeepCall | NestDeepHandle =>
      val closer: Array[U8](7) = " else 0"
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterCenter, 7, closer, depth)
    case NestBinary => afterCenter
    case NestIs =>
      val closer: Array[U8](5) = " is a"
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterCenter, 5, closer, depth)
    case NestCall =>
      val closer: Array[U8](2) = "()"
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterCenter, 2, closer, depth)
    case NestAssert =>
      val closer: Array[U8](7) = "@<p(0)>"
      Stigi.Syntax.Print.Tests.repeatText(bufferCap, sourceBuffer, afterCenter, 7, closer, depth)
  val totalWritten: U64 = if bufferCap < afterClose then bufferCap + 1 else kind match
    case NestType =>
      val tail: Array[U8](11) = "): U64 = 0\n"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterClose, 11, tail)
    case NestDefReturn =>
      val tail: Array[U8](5) = " = 0\n"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterClose, 5, tail)
    case NestPattern | NestPatOr =>
      val tail: Array[U8](6) = " => 0\n"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterClose, 6, tail)
    case NestTypeDecl | NestIf | NestParen | NestBinary | NestIs | NestCall | NestAssert | NestDeepFact | NestVarInvariant | NestLocalExtern | NestDeepCall | NestDeepHandle =>
      val tail: Array[U8](1) = "\n"
      Stigi.Syntax.Print.Tests.appendText(bufferCap, sourceBuffer, afterClose, 1, tail)
  if bufferCap < totalWritten then false else Stigi.Syntax.Print.Tests.checkBufferRoundTrip{region}(bufferCap, sourceBuffer, totalWritten, expectAccept)
