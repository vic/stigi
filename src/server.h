#ifndef STIGI_SERVER_H
#define STIGI_SERVER_H

#include "common.h"
#include "door.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STIGI_MAX_FRAME_BYTES (16U * 1024U * 1024U)
#define STIGI_MAX_BATCH_OPS 1024U

typedef struct {
  char id[16];
  char unit_name[128];
  char desc[256];
  char *fixed_src;
  size_t fixed_len;
} StigiCheckedFix;

/* Permission class a --cc-socks reply states in from-mode (frames.md 7.2). */
typedef enum { STIGI_CC_FROM_PROMPTING = 0, STIGI_CC_FROM_BYPASS = 1 } StigiCcFromMode;

typedef struct {
  StigiDoor *door;
  const char *peer_name;     /* from-name of --cc-socks replies; "stigi" unless --peer */
  StigiCcFromMode from_mode; /* prompting unless --from-mode bypass */
  Arena scratch;
  bool json_mode;
  size_t budget_tokens; /* 0 = unlimited */
  bool stopped;
  StigiCheckedFix *fixes;
  uint32_t fix_count;
  uint32_t fix_cap;
} StigiServerCtx;

void stigi_server_init(StigiServerCtx *ctx, StigiDoor *door);
void stigi_server_free(StigiServerCtx *ctx);

/* Core stateless frame dispatcher (door.md 9 + frames.md S114) */
Status stigi_handle_frame(StigiServerCtx *ctx, const char *frame, size_t len,
                          char **out_reply, size_t *out_len, Diag *diag);

/* Core Unix domain socket server (mode 0600, SO_PEERCRED same-UID, frames.md 7.1) */
Status stigi_serve_socket(StigiServerCtx *ctx, const char *sock_path,
                          uint32_t max_requests, Diag *diag);

/* Client helper to send one frame over a Unix domain socket */
Status stigi_send_socket_frame(const char *sock_path, const char *frame, size_t len,
                               char **out_reply, size_t *out_len, Diag *diag);

/* Claude Code --cc-socks adapter (frames.md 7.2 + opencc/ircc msgV:1 peer protocol) */
Status stigi_cc_socks_handle_envelope(StigiServerCtx *ctx, const char *envelope_json, size_t len,
                                      char **out_reply, size_t *out_len, Diag *diag);
Status stigi_serve_cc_socks(StigiServerCtx *ctx, const char *sock_path,
                            uint32_t max_requests, Diag *diag);
Status stigi_cc_peer_reply_frame(const StigiServerCtx *ctx, const char *my_sock, const char *reply,
                                 size_t rlen, char **out, size_t *out_len, Diag *diag);
/* A peer name that fits the cross-session wrapper attribute: no <>"\ and no control byte. */
bool stigi_cc_peer_name_valid(const char *name);
Status stigi_cc_peer_register(const char *peer_name, const char *sock_path,
                              char *out_reg_path, size_t reg_cap, Diag *diag);

#endif
