// prompt: closed outcome of allocating reusable length-collection scratch buffers.
enum Stigi.Store.LengthScratchMade:
  case LengthScratchReady(scratch: Stigi.Store.LengthScratch)
  case LengthScratchUnavailable
