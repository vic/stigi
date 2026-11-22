// prompt: verify Arena alignment and 1000 reset cycles, Heap size-class LIFO reuse, and 3-transport byte parity.
val Stigi.Tests.Runtime.arenaHeapServerSuite {Stigi.Test}: Stigi.Test.Result =
  val alignedSmall: U64 = Stigi.Arena.alignBlock(13)
  val alignedLine: U64 = Stigi.Arena.alignBlock(70)
  val alignValid: Bool = alignedSmall == 16 && alignedLine == 128
  var cycle: U64 = 0
  var cyclesValid: Bool = true
  while cycle < 1000 do
    @<lessOrEqual(cycle, 1000), decreases(1000 - cycle)>
    val firstAlloc: Stigi.Arena.AllocOutcome = Stigi.Arena.tryAllocate(4096, 0, 24)
    val stepValid: Bool = firstAlloc match
      case ArenaExhausted => false
      case Allocated(offsetOne, nextOne) =>
        Stigi.Arena.usedWithinCapacity(0, nextOne, 4096)
        val secondAlloc: Stigi.Arena.AllocOutcome = Stigi.Arena.tryAllocate(4096, nextOne, 70)
        secondAlloc match
          case ArenaExhausted => false
          case Allocated(offsetTwo, nextTwo) =>
            offsetOne == 0 && nextOne == 24 && offsetTwo == 64 && nextTwo == 192
    if !stepValid then
      cyclesValid = false
    else
      ()
    cycle = cycle + 1
  val oomValid: Bool = Stigi.Arena.tryAllocate(128, 96, 64) match
    case ArenaExhausted => true
    case Allocated(_, _) => false
  val classChecks: Bool = Stigi.Heap.selectSizeClass(12) == ClassSlot(0, 16) && Stigi.Heap.selectSizeClass(64) == ClassSlot(2, 64) && Stigi.Heap.selectSizeClass(4096) == ClassSlot(7, 4096) && Stigi.Heap.selectSizeClass(8192) == ClassOversized
  val zeroWord: U64 = 0
  Array.tryFill{Stigi.Region}(8, zeroWord) match
    case FillFailed => Stigi.Test.expect(false, true, "heap stack alloc failed")
    case Filled(freeSlots) =>
      val countOne: U64 = Stigi.Heap.pushFreeSlot(8, freeSlots, 0, 77)
      val countTwo: U64 = Stigi.Heap.pushFreeSlot(8, freeSlots, countOne, 88)
      val reuseValid: Bool = if countTwo > 0 then Stigi.Heap.popFreeSlot(8, freeSlots, countTwo) == 88 else false
      val coreEnv: Stigi.Server.EnvelopeOutcome = Stigi.Server.unwrapTransportFrame(TransportCoreSocket, 5, "tests")
      val peerEnv: Stigi.Server.EnvelopeOutcome = Stigi.Server.unwrapTransportFrame(TransportPeerSocks, 25, "{\"msgV\":1,\"body\":\"tests\"}")
      val mcpEnv: Stigi.Server.EnvelopeOutcome = Stigi.Server.unwrapTransportFrame(TransportMcpStdio, 33, "{\"method\":\"call\",\"frame\":\"tests\"}")
      val directEnv: Stigi.Server.EnvelopeOutcome = Stigi.Server.extractJsonStringField(15, "{\"name\":\"demo\"}", 4, "name")
      val coreMatch: Bool = coreEnv match
        case EnvelopeInvalid => false
        case EnvelopeSpan(startCore, endCore) => Stigi.Parse.sliceEquals(5, "tests", startCore, endCore, 5, "tests")
      val peerMatch: Bool = peerEnv match
        case EnvelopeInvalid => false
        case EnvelopeSpan(startPeer, endPeer) => Stigi.Parse.sliceEquals(25, "{\"msgV\":1,\"body\":\"tests\"}", startPeer, endPeer, 5, "tests")
      val mcpMatch: Bool = mcpEnv match
        case EnvelopeInvalid => false
        case EnvelopeSpan(startMcp, endMcp) => Stigi.Parse.sliceEquals(33, "{\"method\":\"call\",\"frame\":\"tests\"}", startMcp, endMcp, 5, "tests")
      val directMatch: Bool = directEnv match
        case EnvelopeInvalid => false
        case EnvelopeSpan(startDir, endDir) => Stigi.Parse.sliceEquals(15, "{\"name\":\"demo\"}", startDir, endDir, 4, "demo")
      val allPassed: Bool = alignValid && cyclesValid && oomValid && classChecks && reuseValid && coreMatch && peerMatch && mcpMatch && directMatch
      Stigi.Test.expect(allPassed, true, "Arena 1000 cycles, Heap LIFO reuse, and 3-transport parity must pass")
