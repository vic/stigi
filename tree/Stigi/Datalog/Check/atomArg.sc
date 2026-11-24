// prompt: return the term at a proved argument slot of a Datalog atom.
pub def Stigi.Datalog.Check.atomArg(atom: Stigi.Datalog.Check.Atom, slot: U64) <below(slot, 7)>: Stigi.Datalog.Check.Term =
  atom match
    case AtomRef(_, _, argZero, argOne, argTwo, argThree, argFour, argFive, argSix) =>
      if slot == 0 then
        argZero
      else if slot == 1 then
        argOne
      else if slot == 2 then
        argTwo
      else if slot == 3 then
        argThree
      else if slot == 4 then
        argFour
      else if slot == 5 then
        argFive
      else
        argSix
