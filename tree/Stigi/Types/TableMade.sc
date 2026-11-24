// prompt: the outcome of allocating one type table: the empty table, or an array could not be allocated.
enum Stigi.Types.TableMade:
  case TableReady(table: Stigi.Types.TypeTable)
  case TableUnavailable
