// prompt: the outcome of exporting a tree: the count of units and of edges found with the image and the edge table that hold them, or why the export failed, with the index, the name and the step of the unit that stopped it when one did.
pub enum Stigi.Names.ExportOutcome:
  case Exported(units: U64, edges: U64, image: Stigi.Names.Image, edgeTable: Stigi.Names.Edges)
  case ExportFailed(why: Stigi.Names.TreeFailure)
  case ExportFailedAt(entry: U64, step: Stigi.Names.ExportStep, why: Stigi.Names.TreeFailure, nameBuf: Array[U8](Stigi.Limit.maxUnitBytes), nameSize: U64) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes)>
