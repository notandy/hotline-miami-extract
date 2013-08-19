/* gcc -o extract extract.c */

/*
** Extract everything from Hotline Miami WAD
**
** Coypright (C) 2013 Andrew K. <andy@mailbox.tu-berlin.de>
** idea & soundtrack extractor from
**  Ernst Friedrich Jonathan Wonneberger <info@jonwon.de>
**  Sebastian Pipping <sebastian@pipping.org>
**
** Licensed under GPL v3
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

struct files {
    char *filename;
    uint32_t filename_len;
    uint32_t file_len;
    uint32_t offset;
};

static int mkdir_report(const char * dir) {
    const mkdir_res = mkdir(dir, S_IRWXU);
    if(mkdir_res == 0) {
        printf("Directory \"%s\" created.\n", dir);
    }
    return mkdir_res;
}

static void _mkdir(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;
 
    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
        if(*p == '/') {
            *p = 0;
            mkdir_report(tmp);
            *p = '/';
        }
    mkdir_report(tmp);
}

int main(int argc, char *argv[]) {
    struct files *files_array;
    struct stat s;
    void *file_ptr;
    uint32_t header_end, num_files, i; 
    int fd;
    char *cur_header_offset;
    

    if(argc != 2)
        goto usage;

    fd = open(argv[1], O_RDONLY);
    if(fd < 0)
        goto usage;

    const int fstat_res = fstat(fd, &s);
    if(fstat_res == -1)
    {
        fprintf(stderr, "Statting failed\n");
        return EXIT_FAILURE;
    }

    file_ptr = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if(file_ptr == MAP_FAILED)
    {
        fprintf(stderr, "Allocation failed\n");
        return EXIT_FAILURE;
    }

    header_end = ((uint32_t *)file_ptr)[0];
    num_files = ((uint32_t *)file_ptr)[1];

    // Header must be at least (4 + 1 + 4 + 4) bytes per contained file
    if(header_end >= s.st_size || header_end < 8 + num_files * 13)
    {
        fprintf(stderr, "Bad header, wrong file?\n");
        return EXIT_FAILURE;
    }

    printf("Found %u files\n", num_files);
    cur_header_offset = 8 + (char *)file_ptr;

    files_array = malloc(sizeof(struct files) * num_files);

    for(i = 0; i < num_files; i++)
    {
        files_array[i].filename_len = *(uint32_t *)cur_header_offset;
        cur_header_offset += 4;
        
        files_array[i].filename = cur_header_offset;
        cur_header_offset += files_array[i].filename_len;
        
        files_array[i].file_len = *(uint32_t *)cur_header_offset;
        cur_header_offset += 4; 
        
        files_array[i].offset = *(uint32_t *)cur_header_offset;
        cur_header_offset += 4; 
    }

    for(i = 0; i < num_files; i++)
    {
        char *filepath = strndup(files_array[i].filename,
                files_array[i].filename_len);
        char *filedir = dirname(strdup(filepath));

        if(stat(filedir, &s))
        {
            _mkdir(filedir);
        }

        if((fd = open(filepath, O_WRONLY|O_CREAT|O_EXCL, 
            S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) == -1)
        {
            fprintf(stderr, "Error creating file %s\n", filepath);
            return EXIT_FAILURE;
        }

        if(write(fd, cur_header_offset + files_array[i].offset, 
                    files_array[i].file_len) == files_array[i].file_len)
        {
            printf("File \"%s\" (size %u bytes) extracted.\n", 
                    filepath, files_array[i].file_len);
        }
        else
        {
            fprintf(stderr, "Error writing file %s\n", filepath);
            return EXIT_FAILURE;
        }
        free(filedir);
        free(filepath);
        close(fd);
    }
    free(files_array);
    return EXIT_SUCCESS;

usage:
    printf("Usage: %s [FILE]\n", argv[0]);
    return EXIT_FAILURE;    
}

