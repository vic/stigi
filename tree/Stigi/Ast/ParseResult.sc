// prompt: closed outcome of parsing one unit token stream into header and flat AST nodes.
pub enum Stigi.Ast.ParseResult:
  case ParsedUnit(header: Stigi.Ast.UnitHeader, nodeCount: U64)
  case ParseFailed(tokIndex: U64)
