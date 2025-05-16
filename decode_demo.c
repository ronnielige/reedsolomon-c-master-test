#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs.h"

// 检查是否全为 0（标记为丢失 shard 的方法）
int is_zero_block(const unsigned char* buf, int size) {
    for (int i = 0; i < size; i++) {
        if (buf[i] != 0) return 0;
    }
    return 1;
}

unsigned char* decode_rs_from_buffer(
    const unsigned char* encoded_buf, int encoded_size,
    int* out_file_size)
{
    if (!encoded_buf || encoded_size < sizeof(int) * 4) {
        fprintf(stderr, "Invalid buffer.\n");
        return NULL;
    }

    const unsigned char* ptr = encoded_buf;
    int data_shards = *((int*)ptr); ptr += sizeof(int);
    int parity_shards = *((int*)ptr); ptr += sizeof(int);
    int block_size = *((int*)ptr); ptr += sizeof(int);
    int file_size = *((int*)ptr); ptr += sizeof(int);
    int total_shards = data_shards + parity_shards;

    if (encoded_size < sizeof(int) * 4 + total_shards * block_size) {
        fprintf(stderr, "Encoded buffer too small.\n");
        return NULL;
    }

    // 分配 shard 指针和标记
    unsigned char** shards = malloc(sizeof(unsigned char*) * total_shards);
    unsigned char* marks = calloc(total_shards, 1);  // 0=ok, 1=lost

    // 读取所有 shard
    for (int i = 0; i < total_shards; i++) {
        shards[i] = malloc(block_size);
        memcpy(shards[i], ptr, block_size);
        ptr += block_size;

        if (is_zero_block(shards[i], block_size)) {
            free(shards[i]);
            shards[i] = NULL;
            marks[i] = 1;
        }
    }

    // 检查是否可恢复
    int available = 0;
    for (int i = 0; i < total_shards; i++) {
        if (shards[i]) available++;
    }
    if (available < data_shards) {
        fprintf(stderr, "ERROR: Not enough shards to recover. Needed %d, got %d\n", data_shards, available);
        goto fail;
    }

    // 分配用于恢复的 shard
    for (int i = 0; i < total_shards; i++) {
        if (shards[i] == NULL) {
            shards[i] = malloc(block_size);
        }
    }

    // 解码
    fec_init();
    reed_solomon* rs = reed_solomon_new(data_shards, parity_shards);
    if (!rs || reed_solomon_reconstruct(rs, shards, marks, total_shards, block_size) != 0) {
        fprintf(stderr, "Reconstruction failed.\n");
        goto fail;
    }

    // 提取原始数据
    unsigned char* out_data = malloc(file_size);
    int copied = 0;
    for (int i = 0; i < data_shards && copied < file_size; i++) {
        int remain = file_size - copied;
        int copy_len = (remain < block_size) ? remain : block_size;
        memcpy(out_data + copied, shards[i], copy_len);
        copied += copy_len;
    }

    // 清理
    for (int i = 0; i < total_shards; i++) free(shards[i]);
    free(shards);
    free(marks);
    reed_solomon_release(rs);
    *out_file_size = file_size;
    return out_data;

fail:
    for (int i = 0; i < total_shards; i++) if (shards[i]) free(shards[i]);
    free(shards);
    free(marks);
    return NULL;
}

// 用法示例 main()
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_bin> <output_file>\n", argv[0]);
        return -1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    FILE* fp = fopen(input_file, "rb");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int buf_size = ftell(fp);
    rewind(fp);
    unsigned char* buf = malloc(buf_size);
    fread(buf, 1, buf_size, fp);
    fclose(fp);

    int decoded_size = 0;
    unsigned char* decoded = decode_rs_from_buffer(buf, buf_size, &decoded_size);
    free(buf);

    if (!decoded) {
        fprintf(stderr, "Decoding failed.\n");
        return -1;
    }

    FILE* fout = fopen(output_file, "wb");
    fwrite(decoded, 1, decoded_size, fout);
    fclose(fout);
    free(decoded);

    printf("Decoded and saved to: %s\n", output_file);
    return 0;
}
