// prompt: write the words of one tree failure reason and a line end into the report buffer at a position, giving the new end.
def Stigi.Store.putReasonWord {Stigi.Mut} (report: Array[U8](65536), at: U64, why: Stigi.Names.TreeFailure) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  why match
    case SourceRejected => Stigi.Array.copy[U8](16, "source rejected\n", 0, 16, 65536, report, at)
    case NameMismatch => Stigi.Array.copy[U8](14, "name mismatch\n", 0, 14, 65536, report, at)
    case ImageExhausted => Stigi.Array.copy[U8](16, "image exhausted\n", 0, 16, 65536, report, at)
    case WalkFaulted(ReasonStack) => Stigi.Array.copy[U8](16, "walk stack full\n", 0, 16, 65536, report, at)
    case WalkFaulted(ReasonScope) => Stigi.Array.copy[U8](16, "walk scope full\n", 0, 16, 65536, report, at)
    case WalkFaulted(ReasonTable) => Stigi.Array.copy[U8](21, "walk table malformed\n", 0, 21, 65536, report, at)
    case WalkFaulted(ReasonSteps) => Stigi.Array.copy[U8](21, "walk steps exhausted\n", 0, 21, 65536, report, at)
    case WalkFaulted(ReasonEdges) => Stigi.Array.copy[U8](16, "walk edges full\n", 0, 16, 65536, report, at)
    case UnresolvedNames => Stigi.Array.copy[U8](17, "unresolved names\n", 0, 17, 65536, report, at)
    case BufferUnavailable => Stigi.Array.copy[U8](19, "buffer unavailable\n", 0, 19, 65536, report, at)
    case TreeUnreadable => Stigi.Array.copy[U8](16, "unit unreadable\n", 0, 16, 65536, report, at)
    case LinesExhausted => Stigi.Array.copy[U8](22, "index lines exhausted\n", 0, 22, 65536, report, at)
