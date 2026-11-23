// prompt: nominal capacity refinement for emitted C output buffers.
type Stigi.EmitCap = U64 <below(self, Stigi.Limit.maxEmitBytes)>
