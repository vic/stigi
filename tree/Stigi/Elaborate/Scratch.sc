// prompt: the reused working arrays of one elaboration: the work stack with its top, the syntax-to-core map and the pattern resolutions, all of one capacity.
pub enum Stigi.Elaborate.Scratch:
  case WorkScratch(capacity: U64, work: Array[Stigi.Elaborate.Work](capacity), top: U64, mapped: Array[U64](capacity), patRes: Array[Stigi.Elaborate.PatResolution](capacity)) <lessOrEqual(top, capacity)>
