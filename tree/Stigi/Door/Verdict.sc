// prompt: closed admission verdict returned by the self-hosted Stigi door.
pub enum Stigi.Door.Verdict:
  case DoorAccepted(nodeCount: U64, factCount: U64, derivedRows: U64)
  case DoorGateFailed
  case DoorLexFailed
  case DoorParseFailed
  case DoorDatalogRejected(rejectCount: U64)
  case DoorOutOfMemory
