// prompt: represent the outcome of allocating elaboration working scratch.
pub enum Stigi.Elaborate.ScratchMade:
  case ScratchReady(scratch: Stigi.Elaborate.Scratch)
  case ScratchUnavailable
