// prompt: classify every protocol and agent frame supported by the Stigi door server.
pub enum Stigi.Door.FrameKind:
  case FrameBatch
  case FrameCheck
  case FrameAccept
  case FrameSummary
  case FrameBudget
  case FrameSubmit
  case FrameDrop
  case FrameShow
  case FrameFind
  case FrameList
  case FrameQuery
  case FrameLaws
  case FrameStore
  case FrameLoad
  case FrameExport
  case FrameEmit
  case FrameTests
  case FrameJson
  case FrameStop
  case FrameUnknown
