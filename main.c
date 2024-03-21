#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

uint32_t crc32b(uint8_t *data, size_t size) {
   uint32_t byte, crc, mask;
   crc = 0xFFFFFFFF;
   for (int i = 0; i < size; i++) {
      byte = data[i];           
        crc = crc ^ byte;
        for (int j = 7; j >= 0; j--) {  
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
   }
   return ~crc;
}

void *load_file(const char* path, size_t *size) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) return NULL; 

    *size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    void *map = mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    close(fd);
    return map;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("[*] usage: ./cwm_fov <input_file> <output_file> <fov>\n");
        return -1;
    }

    int target_fov = atoi(argv[3]);
    if (target_fov >= 128) {
        printf("[-] fov size is too large (range: 0 - 127)\n");
        return -1;
    }

    size_t file_size = 0x0;
    void *file_data = load_file(argv[1], &file_size);
    if (file_data == NULL) {
        printf("[-] failed to load file data for: %s\n", argv[1]);
        return -1;
    }

    uint32_t old_crc32 = *(uint32_t *)(file_data+file_size-4);
    uint8_t old_fov = (*(uint8_t *)(file_data+0xc7d)) / 2;

    *(uint8_t *)(file_data+0xc7d) = target_fov * 2;
    *(uint32_t *)(file_data+file_size-4) = crc32b(file_data, file_size-4);

    uint32_t new_crc32 = *(uint32_t *)(file_data+file_size-4);
    uint8_t new_fov = (*(uint8_t *)(file_data+0xc7d)) / 2;

    printf("[*] crc32: 0x%x -> 0x%x\n", old_crc32, new_crc32);
    printf("[*] fov: %d -> %d\n", old_fov, new_fov);

    FILE *output = fopen(argv[2], "w+");
    if (output == NULL) {
        printf("[-] failed to open output file\n");
        return -1;
    }

    fwrite(file_data, file_size, 0x1, output);
    fclose(output);
    return 0;
}
