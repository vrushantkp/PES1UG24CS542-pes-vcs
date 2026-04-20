#include "pes.h"
#include "index.h"
#include "commit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <inttypes.h>

// Forward declarations
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int head_read(ObjectID *id_out);
int head_update(const ObjectID *new_commit);
int commit_walk(commit_walk_fn callback, void *ctx);

// ─── cmd_init ────────────────────────────────────────────────────────────────

void cmd_init(void) {
    // Create directory structure
    mkdir(PES_DIR,           0755);
    mkdir(OBJECTS_DIR,       0755);
    mkdir(".pes/refs",       0755);
    mkdir(REFS_DIR,          0755);

    // Write HEAD pointing to main branch
    FILE *f = fopen(HEAD_FILE, "w");
    if (!f) { perror("HEAD"); return; }
    fprintf(f, "ref: refs/heads/main\n");
    fclose(f);

    printf("Initialized empty PES repository in .pes/\n");
}

// ─── cmd_add ─────────────────────────────────────────────────────────────────

void cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: pes add <file>...\n");
        return;
    }

    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }

    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) != 0) {
            fprintf(stderr, "error: failed to add '%s'\n", argv[i]);
        }
    }
}

// ─── cmd_status ──────────────────────────────────────────────────────────────

void cmd_status(void) {
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }
    index_status(&index);
}

// ─── cmd_commit ──────────────────────────────────────────────────────────────

void cmd_commit(int argc, char *argv[]) {
    const char *message = NULL;
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            message = argv[i + 1];
            break;
        }
    }

    if (!message) {
        fprintf(stderr, "error: commit requires a message (-m \"message\")\n");
        return;
    }

    ObjectID commit_id;
    if (commit_create(message, &commit_id) != 0) {
        fprintf(stderr, "error: commit failed\n");
        return;
    }

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit_id, hex);
    printf("Committed: %.12s... %s\n", hex, message);
}

// ─── cmd_log ─────────────────────────────────────────────────────────────────

static void log_callback(const ObjectID *id, const Commit *c, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    time_t ts = (time_t)c->timestamp;
    char time_buf[64];
    struct tm *tm_info = localtime(&ts);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("commit %s\n", hex);
    printf("Author: %s\n", c->author);
    printf("Date:   %s\n", time_buf);
    printf("\n    %s\n\n", c->message);
}

void cmd_log(void) {
    if (commit_walk(log_callback, NULL) != 0) {
        fprintf(stderr, "error: no commits yet (or failed to read history)\n");
    }
}

// ─── PROVIDED: Phase 5 Command Wrappers ─────────────────────────────────────
// (branch and checkout are stubs for analysis-only phases)

void cmd_branch(int argc, char *argv[]) {
    (void)argc; (void)argv;
    fprintf(stderr, "branch: not implemented (analysis-only phase)\n");
}

void cmd_checkout(int argc, char *argv[]) {
    (void)argc; (void)argv;
    fprintf(stderr, "checkout: not implemented (analysis-only phase)\n");
}

// ─── Command dispatch ────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pes <command> [args]\n");
        fprintf(stderr, "\nCommands:\n");
        fprintf(stderr, "  init              Create a new PES repository\n");
        fprintf(stderr, "  add <file>...     Stage files for commit\n");
        fprintf(stderr, "  status            Show working directory status\n");
        fprintf(stderr, "  commit -m <msg>   Create a commit from staged files\n");
        fprintf(stderr, "  log               Show commit history\n");
        return 1;
    }

    const char *cmd = argv[1];
    if      (strcmp(cmd, "init")     == 0) cmd_init();
    else if (strcmp(cmd, "add")      == 0) cmd_add(argc, argv);
    else if (strcmp(cmd, "status")   == 0) cmd_status();
    else if (strcmp(cmd, "commit")   == 0) cmd_commit(argc, argv);
    else if (strcmp(cmd, "log")      == 0) cmd_log();
    else if (strcmp(cmd, "branch")   == 0) cmd_branch(argc, argv);
    else if (strcmp(cmd, "checkout") == 0) cmd_checkout(argc, argv);
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }
    return 0;
}
