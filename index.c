#include "index.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_FILE ".pes/index"

// Load index from disk
int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) {
        // No index yet → valid state
        return 0;
    }

    // Check file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size == 0) {
        fclose(f);
        return 0; // empty index is fine
    }

    // Read entries
    while (!feof(f)) {
        IndexEntry entry;
        if (fread(&entry, sizeof(IndexEntry), 1, f) == 1) {
            index->entries[index->count++] = entry;
        }
    }

    fclose(f);
    return 0;
}

// Save index to disk
int index_save(Index *index) {
    FILE *f = fopen(INDEX_FILE, "wb");
    if (!f) return -1;

    fwrite(index->entries, sizeof(IndexEntry), index->count, f);
    fclose(f);
    return 0;
}

// Add file to index
int index_add(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("file open failed");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(size);
    if (!data) {
        fclose(f);
        return -1;
    }

    fread(data, 1, size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, size, &id) != 0) {
        perror("object_write failed");
        free(data);
        return -1;
    }

    free(data);

    IndexEntry entry;
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.path, path, sizeof(entry.path) - 1);
    entry.id = id;

    index->entries[index->count++] = entry;

    if (index_save(index) != 0) {
        perror("index_save failed");
        return -1;
    }

    return 0;
}

// Print status
void index_status(Index *index) {
    if (index->count == 0) {
        printf("No files staged.\n");
        return;
    }

    printf("Staged files:\n");
    for (int i = 0; i < index->count; i++) {
        printf("  %s\n", index->entries[i].path);
    }
}
