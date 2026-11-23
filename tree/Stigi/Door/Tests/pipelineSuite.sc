// prompt: verify full self-hosted door pipeline accepts valid units and rejects missing prompts.
val Stigi.Door.Tests.pipelineSuite {Stigi.Test}: Stigi.Test.Result =
  val goodUnit: Array[U8](45) = "// prompt: ok.\ndef Demo.add(a: U64) = a < 10\n"
  val badUnit: Array[U8](25) = "def Demo.noHeader(a) = 1\n"
  val acceptValid: Bool = Stigi.Door.admitUnit{Stigi.Region}(45, goodUnit, 45) match
    case DoorAccepted(nodeCount, factCount, derivedRows) => nodeCount >= 3 && factCount >= 1 && derivedRows >= 2
    case DoorGateFailed => false
    case DoorLexFailed => false
    case DoorParseFailed => false
    case DoorDatalogRejected(_) => false
    case DoorOutOfMemory => false
  val rejectGate: Bool = Stigi.Door.admitUnit{Stigi.Region}(25, badUnit, 25) match
    case DoorGateFailed => true
    case DoorAccepted(_, _, _) => false
    case DoorLexFailed => false
    case DoorParseFailed => false
    case DoorDatalogRejected(_) => false
    case DoorOutOfMemory => false
  Stigi.Test.expect(acceptValid && rejectGate, true, "self-hosted door must run full pipeline and enforce tenet gate")
