// prompt: the token after a token, or the token itself at the end of the token table.
def Stigi.Syntax.Parse.nextToken(token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: U64 =
  if token < Stigi.Limit.maxUnitBytes then token + 1 else token
