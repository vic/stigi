// prompt: one slot of the hash index over interned types: empty, or holding the index of an interned type.
enum Stigi.Types.TypeSlot:
  case EmptySlot
  case TypeHeld(typeAt: Stigi.Core.TypeAt)
