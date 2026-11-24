// prompt: the next free entry of the node table, or a full table.
enum Stigi.Syntax.Parse.Slot:
  case FreeSlot(at: Stigi.Syntax.NodeAt)
  case TableFull
