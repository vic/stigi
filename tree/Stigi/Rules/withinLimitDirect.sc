// prompt: derive withinLimit directly from a proved strict upper bound.
rule def Stigi.Rules.withinLimitDirect: Stigi.Rules.withinLimit(valNode, limNode) =
  below(valNode, limNode, scopeNode)
