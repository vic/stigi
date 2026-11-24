// prompt: buffers one tree check reuses for every unit: path, index text, unit source, tokens, nodes, facts, rows and rules.
enum Stigi.Store.Scratch:
  case Scratch(path: Array[U8](4096), index: Array[U8](65536), unit: Array[U8](65536), toks: Array[Stigi.Lex.Token](16384), nodes: Array[Stigi.Ast.Node](16384), facts: Array[Stigi.Extract.FactRow](16384), rows: Array[Stigi.Datalog.TupleRow](16384), rules: Array[Stigi.Datalog.RuleSpec](4))
