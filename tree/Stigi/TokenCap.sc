// prompt: nominal capacity refinement for token arrays.
type Stigi.TokenCap = U64 <below(self, Stigi.Limit.maxTokens)>
