#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_FILE ".pes/index"

// Load index
int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size == 0) {
        fclose(f);
        return 0;
    }

    fread(index->entries, sizeof(IndexEntry), MAX_INDEX_ENTRIES, f);
    index->count = size / sizeof(IndexEntry);

    fclose(f);
    return 0;
}

// Save index
int index_save(const Index *index) {
    FILE *f = fopen(INDEX_FILE, "wb");
    if (!f) return -1;

    fwrite(index->entries, sizeof(IndexEntry), index->count, f);
    fclose(f);
    return 0;
}

// Add file
int index_add(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    IndexEntry entry;
    memset(&entry, 0, sizeof(entry));

    strncpy(entry.path, path, sizeof(entry.path) - 1);

    // IMPORTANT: your struct likely uses 'oid', not 'id'
    entry.oid = id;

    index->entries[index->count++] = entry;

    return index_save(index);
}

// Status
int index_status(const Index *index) {
    if (index->count == 0) {
        printf("Staged changes:\n  (nothing to show)\n\n");
    } else {
        printf("Staged changes:\n");
        for (int i = 0; i < index->count; i++) {
            printf("  %s\n", index->entries[i].path);
        }
        printf("\n");
    }

    // Simplified untracked listing (optional)
    printf("Untracked changes:\n  (not fully implemented)\n\n");

    return 0;
}
