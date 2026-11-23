// prompt: nominal capacity refinement for filesystem path arrays.
type Stigi.PathCap = U64 <below(self, Stigi.Limit.maxPathBytes)>
