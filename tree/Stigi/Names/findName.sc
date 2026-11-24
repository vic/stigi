// prompt: find the unit of an image that carries the name a candidate stands for.
def Stigi.Names.findName(image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, candidate: Stigi.Names.Candidate) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Lookup =
  image match
    case Image(pool, _, _, _) => Stigi.Names.probeName(image, tables, candidate, Stigi.Names.candidateHash(pool, tables, candidate) % Stigi.Names.tableSlots, Stigi.Names.tableSlots)
