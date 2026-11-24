// prompt: place a key in the first vacant slot of a linear-probing table, false when no slot is left to try.
def Stigi.Names.placeKey {Stigi.Mut} (slots: Array[Stigi.Names.KeySlot](Stigi.Names.tableSlots), key: U64, slot: U64, remaining: U64) <below(slot, Stigi.Names.tableSlots), decreases(remaining)>: Bool =
  if remaining < 1 then false else
    slots(slot) match
      case Vacant =>
        slots(slot) := Keyed(key)
        true
      case Keyed(_) => Stigi.Names.placeKey(slots, key, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
