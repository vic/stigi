// prompt: clock and process telemetry capability protocol.
capability Stigi.Clock:
  def monotonicStamp {region: Stigi.Region, Stigi.Mut} (): Stigi.Clock.Stamp
  def processUsage {region: Stigi.Region, Stigi.Mut} (): Stigi.Clock.Usage
