// prompt: nominal capacity refinement for source byte arrays.
type Stigi.ByteCap = U64 <below(self, Stigi.Limit.maxSourceBytes)>
