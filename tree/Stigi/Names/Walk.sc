// prompt: the counters of a Tarjan walk: the clock, the components found, and the tops of the component and frame stacks.
enum Stigi.Names.Walk:
  case Walk(clock: U64, comps: U64, sccTop: U64, dfsTop: U64)
