// prompt: the outcome of staging rows at the end of a type table list: the table with the span of the new rows, a full list, a length that is not an unsigned linear form, or a binder outside the unit.
enum Stigi.Types.Staged:
  case StagedRows(table: Stigi.Types.TypeTable, rows: Stigi.Core.Span)
  case StageFull
  case StageNotUnsigned
  case StageBadBinder
