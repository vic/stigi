// prompt: outcome of mapping a Heap request size to a fixed power-of-two size class.
pub enum Stigi.Heap.SizeClassOutcome:
  case ClassSlot(classIndex: U64, blockBytes: U64)
  case ClassOversized
