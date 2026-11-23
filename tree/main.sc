// prompt: compiler binary entrypoint dispatching CLI commands and live POSIX contracts.
pub def main {region: Stigi.Region, Stigi.Mut} (): I32 <lessOrEqual(0, result)> =
  handle clock = Stigi.Posix.Clock()
  Stigi.Store.runCli{region, clock}() |> Stigi.outcomeStatus
