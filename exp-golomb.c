#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>    //basename

#include <fcntl.h>     //open
#include <sys/mman.h>  //mmap
#include <sys/stat.h>

#define MAX_ORDER 7    // must be smaller than 24 
#define BUFFER_SIZE 100
#define WRITE_SIZE (BUFFER_SIZE - 2)

/* function status */
#define FAIL 0
#define SUCCESS 1

int encode(int, FILE*, int);
int decode(int, FILE*, int);

int main(int argc, char *argv[])
{
    if (argc < 3) {
        puts("argument not enough");
        return 0;
    }
    
    /* open argv[1] as input file */
    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0) {
        puts("cannot open input file");
        return 0;
    }

    /* open argv[2] as output file */
    FILE *out_file = fopen(argv[2], "wb");
    if (!out_file) {
        puts("cannot open output file");
        return 0; 
    }

    /* use argv[3] as order */
    long int order = 0;
    if (argc >= 4) {
        char* endptr;
        order = strtol(argv[3], &endptr, 10);
        if (endptr == argv[3]) {
            puts("order is not a decimal number");
            return 0;
        }
        if (order > MAX_ORDER) {
            puts("order larger than max order");
            return 0;
        }
    }

    if (strcmp(basename(argv[0]), "encode") == 0) {
        encode(in_fd, out_file, order);
    } else {
        decode(in_fd, out_file, order);
    }
    fclose(out_file);
    return 0;
}

int encode(int in_fd, FILE* out_file, int order)
{
    struct stat s;
    if (fstat(in_fd, &s) < 0) {
        puts("cannot get status of iniput file");
        return FAIL;
    }
    
    uint8_t *map = (uint8_t*) mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
    if (map == MAP_FAILED) {
        puts("cannot open mmap");
        return FAIL;
    }
    
    uint32_t buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int buffer_index = 0;
    int cursor = 32;
    uint32_t offset = 1 << order;

    /* start to encode */
    for (long int i = 0; i < s.st_size; i++) {
        uint32_t current = (uint32_t) map[i];
        current += offset;
        int clz = __builtin_clz(current);
        int unary_width = 31 - clz - order;
        int binary_width = 32 - clz;
        
        /* write unary */
        cursor -= unary_width;
        if (cursor <= 0) {
            cursor += 32;
            buffer_index++;
        }
        
        /* write binary */
        if (cursor < binary_width) {    // truncate in uint32_t
            buffer[buffer_index++] |= current >> (binary_width - cursor);
            buffer[buffer_index] |= current << (32 - (binary_width - cursor));
            cursor = cursor + 32 - binary_width;
        } else {
            buffer[buffer_index] |= current << (cursor - binary_width);
            cursor -= binary_width;
        }

        /* write buffer */
        if (buffer_index >= WRITE_SIZE) {
            fwrite(buffer, 4, WRITE_SIZE, out_file);
            uint32_t tail = buffer[buffer_index];
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = tail;
            buffer_index = 0;
        }
    }

    /* finalize */
    buffer[buffer_index + 1] = (uint32_t) 1; //end signal
    fwrite(buffer, 4, buffer_index + 2, out_file);
    return SUCCESS;
}

int decode(int in_fd, FILE* out_file, int order)
{
    struct stat s;
    if (fstat(in_fd, &s) < 0) {
        puts("cannot get status of iniput file");
        return FAIL;
    }
    
    uint32_t *map = (uint32_t*) mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
    if (map == MAP_FAILED) {
        puts("cannot open mmap");
        return FAIL;
    }

    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int buffer_index = 0;
    int cursor = 32;
    uint32_t offset = 1 << order;
    int map_index = 0;
    uint32_t current = map[0];
    
    while (1) {
        /* read unary */
        int unary_width = 0;
        if (current == 0) {             // truncate in unary field
            current = map[++map_index];
            int clz = __builtin_clz(current);
            unary_width = cursor + clz;
            cursor = 32 - clz;
        } else {
            int clz = __builtin_clz(current);
            unary_width = clz - (32 - cursor);
            cursor = 32 - clz;
        }

        /* end of file is (uint32_t) 1 ,
         * the leading zero of end of file is 31 */
        if (unary_width >= 31) {
            break;
        }

        /* read binary */
        int binary_width = unary_width + 1 + order; 
        uint32_t tmp = 0;
        if (binary_width > cursor) {    // truncate in binary field
            tmp = current << (binary_width - cursor);
            current = map[++map_index];
            binary_width -= cursor;
            cursor = 32;
        }
        tmp |= current >> (cursor - binary_width);
        tmp -= offset;
        buffer[buffer_index++] = (uint8_t) tmp;
        
        /* be careful for left shift 32 bits */
        cursor -= binary_width;
        if (cursor == 0) {
            current = 0;
        } else {
            int shift = 32 - cursor;
            current = (current << shift) >> shift;
        }
        if (buffer_index == BUFFER_SIZE) {
            fwrite(buffer, 1, BUFFER_SIZE, out_file);
            buffer_index = 0;
        }
    }

    /* finalize */
    if (buffer_index != 0) {
        fwrite(buffer, 1, buffer_index, out_file);
    }
    return SUCCESS;
}