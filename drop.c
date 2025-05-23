#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 判断当前 shard 是否需要丢弃
int is_lost(int index, int* lost_list, int lost_count) {
    for (int i = 0; i < lost_count; i++) {
        if (lost_list[i] == index) return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("Usage: %s <input_file.bin> <output_file.bin> <block_size> <shard_id1> [shard_id2] ...\n", argv[0]);
        return -1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];
    int block_size = atoi(argv[3]);

    int lost_count = argc - 4;
    int* lost_list = malloc(sizeof(int) * lost_count);
    if (!lost_list) {
        perror("malloc");
        return -1;
    }
    for (int i = 0; i < lost_count; i++) {
        lost_list[i] = atoi(argv[i + 4]);
    }

    FILE* fp_in = fopen(input_file, "rb");
    if (!fp_in) {
        perror("fopen input");
        free(lost_list);
        return -1;
    }

    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Dropping shards:");
    for (int i = 0; i < lost_count; i++) {
        printf(" %d", lost_list[i]);
    }
    printf("\n");

    FILE* fp_out = fopen(output_file, "wb");
    if (!fp_out) {
        perror("fopen output");
        fclose(fp_in);
        free(lost_list);
        return -1;
    }


    unsigned char* buffer = malloc(block_size);
    if (!buffer) {
        perror("malloc buffer");
        fclose(fp_in);
        fclose(fp_out);
        free(lost_list);
        return -1;
    }

    for (int i = 0; ;i++) {
        size_t read = fread(buffer, 1, block_size, fp_in);
        if (read != block_size) {
            break;
            fprintf(stderr, "Failed to read shard %d from input file\n", i);
            // 可以根据需求选择退出或处理不完整块，这里选择退出
            free(buffer);
            fclose(fp_in);
            fclose(fp_out);
            free(lost_list);
            return -1;
        }

        if (is_lost(i, lost_list, lost_count)) {
            printf("Shard %d dropped.\n", i);
            memset(buffer, 0, block_size);  // 填零
        }
        fwrite(buffer, 1, block_size, fp_out);
    }

    free(buffer);
    free(lost_list);
    fclose(fp_in);
    fclose(fp_out);

    printf("Saved: %s\n", output_file);
    return 0;
}
