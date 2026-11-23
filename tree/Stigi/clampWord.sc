// prompt: clamp an unsigned word into an ordered closed interval with proved postconditions.
def Stigi.clampWord(value: U64, lowBound: U64, highBound: U64) <lessOrEqual(lowBound, highBound)>: U64 <lessOrEqual(lowBound, result), lessOrEqual(result, highBound)> =
  if value < lowBound then lowBound else if highBound < value then highBound else value
