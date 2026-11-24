// prompt: how one L0 expression starts.
enum Stigi.Parse.ExprLead:
  case LeadHead(kind: Stigi.Ast.ExprKind)
  case LeadBind(kind: Stigi.Ast.ExprKind)
  case LeadHandle
  case LeadOperand
