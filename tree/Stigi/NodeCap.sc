// prompt: nominal capacity refinement for AST node arrays.
type Stigi.NodeCap = U64 <below(self, Stigi.Limit.maxNodes)>
