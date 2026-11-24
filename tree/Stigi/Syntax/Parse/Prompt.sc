// prompt: the prompt text on the first line of a unit, or a unit without it.
enum Stigi.Syntax.Parse.Prompt:
  case PromptAt(text: Stigi.Syntax.Text)
  case PromptAbsent
