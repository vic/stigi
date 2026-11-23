// prompt: nominal capacity refinement for image unit tables.
type Stigi.UnitCap = U64 <below(self, Stigi.Limit.maxUnits)>
