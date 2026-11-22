// prompt: unwrap an incoming frame across core socket, peer socks NDJSON, and MCP JSON-RPC transports.
pub def Stigi.Server.unwrapTransportFrame(transport: Stigi.Server.TransportKind, length: U64, payload: Array[U8](length)) <below(length, 0x100000000)>: Stigi.Server.EnvelopeOutcome =
  transport match
    case TransportCoreSocket => EnvelopeSpan(0, length)
    case TransportPeerSocks => Stigi.Server.extractJsonStringField(length, payload, 4, "body")
    case TransportMcpStdio => Stigi.Server.extractJsonStringField(length, payload, 5, "frame")
