#ifndef STIGI_MCP_H
#define STIGI_MCP_H

#include "common.h"
#include "server.h"

#include <stddef.h>

/* Handle one JSON-RPC 2.0 MCP request (initialize, ping, tools/list, tools/call,
 * resources/list, resources/read). For notifications (no "id"), sets *out_json = NULL,
 * *out_len = 0 and returns STIGI_OK. */
Status stigi_mcp_handle_jsonrpc(StigiServerCtx *ctx, const char *req_json, size_t req_len,
                                char **out_json, size_t *out_len, Diag *diag);

/* Run newline-delimited JSON-RPC 2.0 MCP server on stdin/stdout (in-process) */
Status stigi_serve_mcp_stdio(StigiServerCtx *ctx, Diag *diag);

/* Run newline-delimited JSON-RPC 2.0 MCP stdio bridge forwarding to a live core socket */
Status stigi_serve_mcp_stdio_bridge(const char *sock_path, Diag *diag);

#endif
