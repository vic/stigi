// prompt: verify end-to-end deterministic ISO C11 image compilation across enum, val, def, and Test units.
val Stigi.Tests.Emit.compileImageSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val idxText: Array[U8](42) = "Demo.ModeDemo.limitDemo.pickDemo.Tests.run"
  val srcText: Array[U8](284) = "pub enum Demo.Mode:\n  case Fast\n  case Slow\npub val Demo.limit: U64 = 8\npub def Demo.pick(mode: Demo.Mode) <lessOrEqual(0, 1)>: U64 =\n  mode match\n    case Fast => 1\n    case Slow => 2\nval Demo.Tests.run {Stigi.Test}: Stigi.Test.Result =\n  Stigi.Test.expect(Demo.pick(Fast), 1, \"ok\")\n"
  Array.tryFill{Stigi.Region}(4, zeroWord) match
    case FillFailed => Stigi.Test.expect(false, true, "unit failed")
    case Filled(nameStarts) =>
      Array.tryFill{Stigi.Region}(4, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "unit failed")
        case Filled(nameEnds) =>
          Array.tryFill{Stigi.Region}(4, zeroByte) match
            case FillFailed => Stigi.Test.expect(false, true, "unit failed")
            case Filled(kindCodes) =>
              Array.tryFill{Stigi.Region}(4, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                case Filled(digestLow) =>
                  Array.tryFill{Stigi.Region}(4, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                    case Filled(srcStarts) =>
                      Array.tryFill{Stigi.Region}(4, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                        case Filled(srcEnds) =>
                          Array.tryFill{Stigi.Region}(8192, zeroByte) match
                            case FillFailed => Stigi.Test.expect(false, true, "out failed")
                            case Filled(outOne) =>
                              Array.tryFill{Stigi.Region}(8192, zeroByte) match
                                case FillFailed => Stigi.Test.expect(false, true, "out failed")
                                case Filled(outTwo) =>
                                  nameStarts(0) := 0
                                  nameEnds(0) := 9
                                  kindCodes(0) := 1
                                  digestLow(0) := 10
                                  srcStarts(0) := 0
                                  srcEnds(0) := 44
                                  nameStarts(1) := 9
                                  nameEnds(1) := 19
                                  kindCodes(1) := 5
                                  digestLow(1) := 20
                                  srcStarts(1) := 44
                                  srcEnds(1) := 72
                                  nameStarts(2) := 19
                                  nameEnds(2) := 28
                                  kindCodes(2) := 3
                                  digestLow(2) := 30
                                  srcStarts(2) := 72
                                  srcEnds(2) := 185
                                  nameStarts(3) := 28
                                  nameEnds(3) := 42
                                  kindCodes(3) := 11
                                  digestLow(3) := 40
                                  srcStarts(3) := 185
                                  srcEnds(3) := 284
                                  val lenOne: U64 = Stigi.Emit.compileImage{Stigi.Region}(4, 42, idxText, nameStarts, nameEnds, kindCodes, digestLow, digestLow, digestLow, digestLow, 284, srcText, srcStarts, srcEnds, 8192, outOne)
                                  val lenTwo: U64 = Stigi.Emit.compileImage{Stigi.Region}(4, 42, idxText, nameStarts, nameEnds, kindCodes, digestLow, digestLow, digestLow, digestLow, 284, srcText, srcStarts, srcEnds, 8192, outTwo)
                                  val sameOut: Bool = Stigi.Emit.slicesEqual(8192, outOne, 0, lenOne, 8192, outTwo, 0, lenTwo)
                                  val crashFree: Bool = !Stigi.Emit.hasForbiddenCrashCall(8192, outOne, lenOne)
                                  val good: Bool = lenOne > 2000 && sameOut && crashFree
                                  Stigi.Test.expect(good, true, "self-hosted compileImage must emit deterministic crash-free ISO C11")
