// prompt: verify traits, uKanren implicits, nominal refinements, C-DSL primitives, and laws.
val Stigi.Tests.foundation {Stigi.Test}: Stigi.Test.Result =
  val sameWord: Bool = Stigi.sameValue(42, 42)
  val sameByte: Bool = Stigi.sameValue(U8.ofU64(7), U8.ofU64(7))
  val sameFlag: Bool = Stigi.sameValue(true, true)
  val sameLookup: Bool = Stigi.sameValue(Found(9), Found(9))
  val diffLookup: Bool = !Stigi.sameValue(Found(9), Missing)
  val ordCheck: Bool = Stigi.Order.lessThan(3, 9)
  Stigi.Laws.clampInterval(99, 10, 50)
  Stigi.Laws.withinLimitRule(4, 8, 16)
  val clamped: U64 = Stigi.clampWord(99, 10, 50)
  val mixLow: U64 = Stigi.Raw.mixSlot(0, 65536)
  val rotWord: U32 = Stigi.Raw.rotateRightWord(U32.ofU64(1), U32.ofU64(1))
  val capCheck: Stigi.TokenCap = 128
  val nodeCheck: Stigi.NodeCap = 256
  val unitCheck: Stigi.UnitCap = 64
  val emitCheck: Stigi.EmitCap = 1024
  val pathCheck: Stigi.PathCap = 128
  val capsValid: Bool = capCheck == 128 && nodeCheck == 256 && unitCheck == 64 && emitCheck == 1024 && pathCheck == 128
  val littleValid: Bool =
    val Filled(buf) = Array.tryFill[U8]{Stigi.Region}(16, 0) else
      case FillFailed => false
    Stigi.Raw.writeLittleWord(16, buf, 0, U32.ofU64(0x12345678))
    Stigi.Raw.writeLittleWord(16, buf, 4, U32.ofU64(0x9abcdef0))
    val wordBack: U32 = Stigi.Raw.readLittleWord(16, buf, 0)
    val longBack: U64 = Stigi.Raw.readLittleLong(16, buf, 0)
    wordBack == U32.ofU64(0x12345678) && U64.bitwiseAnd(longBack, 0xffffffff) == 0x12345678
  val allPass: Bool = sameWord && sameByte && sameFlag && sameLookup && diffLookup && ordCheck && clamped == 50 && mixLow == 52655 && rotWord == U32.ofU64(0x80000000) && capsValid && littleValid
  Stigi.Test.expect(allPass, true, "foundation traits, refinements, C-DSL, and laws must hold")
