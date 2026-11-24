// prompt: buffers one tree check reuses for every unit: path, index text, unit source, tokens, nodes, facts, rows, links and hash slots.
enum Stigi.Store.Scratch:
  case Scratch(path: Array[U8](4096), index: Array[U8](262144), unit: Array[U8](65536), toks: Array[Stigi.Lex.Token](16384), nodes: Array[Stigi.Ast.Node](16384), facts: Array[Stigi.Extract.FactRow](16384), rows: Array[Stigi.Datalog.Store.Tuple](16384), nextInRel: Array[Stigi.Datalog.Store.Slot](16384), smallSlots: Array[Stigi.Datalog.Store.Slot](512))
