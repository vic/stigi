// prompt: the reused per-unit tables of the parser and the printer, allocated once per run in one region.
pub enum Stigi.Syntax.Tables:
  case Tables(source: Array[U8](Stigi.Limit.maxUnitBytes), tokens: Array[Stigi.Lex.Token](Stigi.Limit.maxUnitBytes), nodes: Array[Stigi.Syntax.Node](Stigi.Limit.maxUnitNodes), lists: Array[Stigi.Syntax.NodeAt](Stigi.Limit.maxUnitNodes), pending: Array[Stigi.Syntax.NodeAt](Stigi.Limit.maxUnitNodes), output: Array[U8](Stigi.Limit.maxUnitBytes))
