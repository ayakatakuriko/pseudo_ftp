#ifndef UTILITY_H
#define UTILITY_H

// TODO: handle error

#define mem_alloc(ptr, type, size, errno) \
        do { \
                if ((ptr = (type *)malloc(sizeof(type) * (size))) == NULL) { \
                        fprintf(stderr, "Error: Failed to allocate memory.\n"); \
                        exit(errno); \
                } \
        } while (0)

#define file_open(fp, fname, mode, errno) \
        do { \
                if ((fp = fopen(fname, mode)) == NULL) { \
                        fprintf(stderr, "Error: Failed to open file: %s\n", fname); \
                        exit(errno); \
                } \
        } while (0)

#define mem_realloc(ptr, type, size, errno) \
        do { \
                if ((ptr = (type *)realloc(ptr, sizeof(type) * (size))) == NULL) { \
                        fprintf(stderr, "Error: Failed to reallocate memory.\n"); \
                } \
        } while (0)

#endif
