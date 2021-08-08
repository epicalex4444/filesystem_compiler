#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint64_t bytesToSectors(uint64_t bytes) {
    if (bytes % 512) {
        return bytes / 512 + 1;
    }
    return bytes / 512;
}

//gets file length in bytes
uint64_t file_length(int8_t* fileName) {
    FILE* file = fopen(fileName, "r");

    fseek(file, 0L, SEEK_END);
    uint64_t bytes = ftell(file);
    fclose(file);

    return bytes;
}

//adds 0's to align to a sector(512 bytes)
void sectorAlign(FILE* fs) {
    fseek(fs, 0L, SEEK_END);
    uint16_t bytes = 512 - ftell(fs) % 512;

    if (bytes == 512) {
        return;
    }

    uint8_t null = 0;
    for (uint16_t i = 0; i < bytes; ++i) {
        fwrite(&null, sizeof(null), 1, fs);
    }
}

//writes the root folder header
void rootHeader(FILE* fs, uint64_t lba, uint32_t fileNum, uint64_t* fileLengths) {
    uint32_t folderNum = 0;
    uint32_t sectors = bytesToSectors(26 + fileNum * 8 + folderNum * 8);
    int8_t* name = "/";

    fwrite(&sectors, sizeof(sectors), 1, fs);
    fwrite(&fileNum, sizeof(fileNum), 1, fs);
    fwrite(&folderNum, sizeof(folderNum), 1, fs);
    fwrite(name, sizeof(name[0]), strlen(name) + 1, fs);

    uint64_t offset = lba + 1;
    for (uint64_t i = 0; i < fileNum; ++i) {
        fwrite(&offset, sizeof(offset), 1, fs);
        offset += bytesToSectors(fileLengths[i]);
    }

    sectorAlign(fs);
}

//adds file to fs
void addFile(FILE* fs, uint64_t bytes, int8_t* hostName, int8_t* guestName) {
    uint64_t sectors = bytesToSectors(bytes);

    fwrite(&sectors, sizeof(sectors), 1, fs);
    fwrite(guestName, sizeof(guestName[0]), strlen(guestName) + 1, fs);
    sectorAlign(fs);

    FILE* file = fopen(hostName, "r");
    int32_t next;
    uint8_t c;

    while ((next = fgetc(file)) != EOF) {
        c = (uint8_t)next;
        fwrite(&c, sizeof(c), 1, fs);
    }

    fclose(file);
    sectorAlign(fs);
}

//creates the filesystem as a binary file
_Bool createFs(uint32_t fileNum, uint64_t lba, int8_t* outName, int8_t** hostNames, int8_t** guestNames) {
    uint64_t* lengths = malloc(sizeof(uint64_t*));

    for (uint32_t i = 0; i < fileNum; ++i) {
        lengths = (uint64_t*)realloc(lengths, sizeof(uint64_t*) * (i + 1));
        if (!(lengths[i] = file_length(hostNames[i]))) {
            free(lengths);
            fprintf(stderr, "empty file provided\n");
            return 1;
        }
    }

    FILE* fs = fopen(outName, "wb");
    rootHeader(fs, lba, fileNum, lengths);

    for (uint32_t i = 0; i < fileNum; ++i) {
        addFile(fs, lengths[i], hostNames[i], guestNames[i]);
    }

    fclose(fs);
    free(lengths);

    return 0;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: ./fs_compiler lba outName files (file format: hostName,guestName)\n");
        return 1;
    }

    uint32_t fileNum = argc - 3;
    uint64_t lba = atoi(argv[1]);
    int8_t* outName = argv[2];
    int8_t** hostNames = malloc(sizeof(int8_t**) * fileNum);
    int8_t** guestNames = malloc(sizeof(int8_t**) * fileNum);

    for (uint32_t i = 0; i < fileNum; ++i) {
        hostNames[i] = strtok(argv[i + 3], ",");
        guestNames[i] = strtok(NULL, ",");

        FILE* fp = fopen(hostNames[i], "r");
        if (fp == NULL) {
            fprintf(stderr, "invalid host file name\n");
            return 1;
        }
        fclose(fp);
    }

    return createFs(fileNum, lba, outName, hostNames, guestNames);
}
