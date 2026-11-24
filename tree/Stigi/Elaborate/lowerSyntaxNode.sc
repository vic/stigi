// prompt: lower any syntax expression node into Checked-IR tables, resetting the scratch stack top first.
pub def Stigi.Elaborate.lowerSyntaxNode {Stigi.Mut} (tables: Stigi.Syntax.Tables, synRoot: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch) <below(synRoot, Stigi.Limit.maxUnitNodes)>: Stigi.Elaborate.LowerOutcome =
  scratch match
    case WorkScratch(capacity, work, _, mapped, patRes) =>
      Stigi.Elaborate.walkBody(tables, synRoot, core, WorkScratch(capacity, work, 0, mapped, patRes))
