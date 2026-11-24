// prompt: walk every reference of one unit: run the tasks of its stack until it is empty or a fault stops it, binding locals and recording edges and unresolved names.
def Stigi.Names.walkFrom {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Walker =
  var current: Stigi.Names.Walker = walker
  while 0 < Stigi.Names.walkerDepth(current) && Stigi.Names.walkerSound(current) do
    current = Stigi.Names.stepWalk(current, image, members, tables, from)
  current
