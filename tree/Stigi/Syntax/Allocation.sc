// prompt: the outcome of allocating the reused per-unit tables once per run.
pub enum Stigi.Syntax.Allocation:
  case Allocated(tables: Stigi.Syntax.Tables)
  case AllocationFailed
