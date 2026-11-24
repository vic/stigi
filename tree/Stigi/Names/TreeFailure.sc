// prompt: the reasons a tree driver step fails: a unit that does not parse or has no shape, a unit whose index name differs from its declared name, a full image, a walk that stopped early for a named reason, a walk that left a reference without a candidate, an index that ran out of lines, a buffer that could not be allocated, or an index or unit file that could not be read.
pub enum Stigi.Names.TreeFailure:
  case SourceRejected
  case NameMismatch
  case ImageExhausted
  case WalkFaulted(reason: Stigi.Names.WalkReason)
  case UnresolvedNames
  case BufferUnavailable
  case TreeUnreadable
  case LinesExhausted
