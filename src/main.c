#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include "door.h"
#include "emit.h"
#include "mcp.h"
#include "server.h"
#include "store.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STIGI_SEED_VERSION "0.1.0"
#define STIGI_DEFAULT_RULES "vic/spec/datalog/all.dl"

static void print_usage(void) {
  printf("usage:\n"
         "  stigi-seed --version\n"
         "  stigi-seed --serve <sock> [--load <dir>] [--user] [--rules <all.dl>]\n"
         "  stigi-seed --cc-socks <sock> [--load <dir>] [--user] [--rules <all.dl>]\n"
         "  stigi-seed --mcp [--socket <sock>] [--load <dir>] [--user] [--rules <all.dl>]\n"
         "  stigi-seed --load <dir> --emit <out.c> [--user] [--rules <all.dl>]\n");
}

int main(int argc, char **argv) {
  /* Ignore SIGPIPE so disconnected clients never terminate the server */
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = SIG_IGN;
  (void)sigaction(SIGPIPE, &sa, NULL);

  const char *rules_path = STIGI_DEFAULT_RULES;
  const char *serve_sock = NULL;
  const char *cc_sock = NULL;
  const char *mcp_sock = NULL;
  const char *load_dir = NULL;
  const char *emit_path = NULL;
  bool mcp_mode = false;
  bool kernel_mode = true;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--version") == 0) {
      printf("stigi-seed %s\n", STIGI_SEED_VERSION);
      return 0;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage();
      return 0;
    } else if (strcmp(argv[i], "--rules") == 0 && i + 1 < argc) {
      rules_path = argv[++i];
    } else if (strcmp(argv[i], "--serve") == 0 && i + 1 < argc) {
      serve_sock = argv[++i];
    } else if (strcmp(argv[i], "--cc-socks") == 0 && i + 1 < argc) {
      cc_sock = argv[++i];
    } else if (strcmp(argv[i], "--mcp") == 0) {
      mcp_mode = true;
    } else if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
      mcp_sock = argv[++i];
    } else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
      load_dir = argv[++i];
    } else if (strcmp(argv[i], "--emit") == 0 && i + 1 < argc) {
      emit_path = argv[++i];
    } else if (strcmp(argv[i], "--user") == 0) {
      kernel_mode = false;
    } else {
      fprintf(stderr, "stigi-seed: unknown or incomplete option '%s'\n", argv[i]);
      print_usage();
      return 1;
    }
  }

  if (!serve_sock && !cc_sock && !mcp_mode && !(load_dir && emit_path)) {
    fprintf(stderr, "stigi-seed: no mode specified (try --help)\n");
    return 1;
  }

  /* --mcp --socket <path> bridge mode: forwards verbatim frames to a live core socket */
  if (mcp_mode && mcp_sock != NULL) {
    Diag diag;
    diag_clear(&diag);
    Status st = stigi_serve_mcp_stdio_bridge(mcp_sock, &diag);
    if (st != STIGI_OK) {
      fprintf(stderr, "stigi-seed: mcp bridge error: %s\n", diag.msg[0] ? diag.msg : "error");
      return 1;
    }
    return 0;
  }

  StigiDoor door;
  Diag diag;
  diag_clear(&diag);
  Status st = stigi_door_init(&door, rules_path, STIGI_DEFAULT_ROW_BUDGET, kernel_mode, &diag);
  if (st != STIGI_OK) {
    fprintf(stderr, "stigi-seed: init failed: %s\n", diag.msg[0] ? diag.msg : "error");
    return 1;
  }

  /* Pre-load --load <dir> before starting emit or any server mode */
  if (load_dir != NULL) {
    StigiDoorResult res;
    stigi_door_result_init(&res);
    st = stigi_load_dir(&door, load_dir, &res, &diag);
    if (st != STIGI_OK || !res.accepted) {
      fprintf(stderr, "stigi-seed: load '%s' failed: %s\n",
              load_dir, diag.msg[0] ? diag.msg : "rejected");
      for (uint32_t i = 0; i < res.reject_count; i++) {
        fprintf(stderr, "  reject %s %s: %s\n",
                res.rejects[i].subject, res.rejects[i].code, res.rejects[i].msg);
      }
      stigi_door_result_free(&res);
      stigi_door_free(&door);
      return 1;
    }
    stigi_door_result_free(&res);
  }

  if (emit_path != NULL) {
    char *c_code = NULL;
    size_t c_len = 0;
    StigiEmitStats stats;
    st = stigi_emit_c(&door, &c_code, &c_len, &stats, &diag);
    if (st != STIGI_OK) {
      fprintf(stderr, "stigi-seed: emit failed: %s\n", diag.msg[0] ? diag.msg : "error");
      stigi_door_free(&door);
      return 1;
    }
    FILE *fp = fopen(emit_path, "wb");
    if (fp == NULL) {
      fprintf(stderr, "stigi-seed: cannot open '%s' for writing\n", emit_path);
      free(c_code);
      stigi_door_free(&door);
      return 1;
    }
    size_t nw = fwrite(c_code, 1, c_len, fp);
    int crc = fclose(fp);
    free(c_code);
    stigi_door_free(&door);
    if (nw != c_len || crc != 0) {
      fprintf(stderr, "stigi-seed: write/close error on '%s'\n", emit_path);
      return 1;
    }
    return 0;
  }

  StigiServerCtx srv;
  stigi_server_init(&srv, &door);
  if (serve_sock != NULL) {
    st = stigi_serve_socket(&srv, serve_sock, 0, &diag);
  } else if (cc_sock != NULL) {
    st = stigi_serve_cc_socks(&srv, cc_sock, 0, &diag);
  } else if (mcp_mode) {
    st = stigi_serve_mcp_stdio(&srv, &diag);
  }

  if (st != STIGI_OK) {
    fprintf(stderr, "stigi-seed: server error: %s\n", diag.msg[0] ? diag.msg : "error");
    stigi_server_free(&srv);
    stigi_door_free(&door);
    return 1;
  }

  stigi_server_free(&srv);
  stigi_door_free(&door);
  return 0;
}
