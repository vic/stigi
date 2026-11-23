// prompt: close withinLimit upward across any proved non-strict bound.
rule def Stigi.Rules.withinLimitTrans: Stigi.Rules.withinLimit(valNode, topNode) =
  below(valNode, midNode, scopeNode), lessOrEqual(midNode, topNode, scopeNode)
