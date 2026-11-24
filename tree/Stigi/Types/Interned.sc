// prompt: the outcome of interning one type: the table with the index of its one equal row, or the type table is full.
enum Stigi.Types.Interned:
  case InternedAt(table: Stigi.Types.TypeTable, typeAt: Stigi.Core.TypeAt)
  case TypeTableFull
