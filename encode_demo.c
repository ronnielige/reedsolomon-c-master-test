#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs.h"

// 封装编码逻辑的函数
unsigned char* encode_rs_from_buffer(
    const unsigned char* file_data, int file_size,
    float loss_rate, int block_size,
    int* out_encoded_size)
{
    if (block_size <= 0 || loss_rate <= 0.0 || loss_rate >= 1.0 || file_size <= 0) {
        fprintf(stderr, "Invalid parameters.\n");
        return NULL;
    }

    int max_data_shards = (file_size + block_size - 1) / block_size;
    int max_total_shards = 255;
    int raw_parity = (int)(max_data_shards * loss_rate / (1.0 - loss_rate) + 0.5);
    int max_parity_shards = max_total_shards - max_data_shards;
    int parity_shards = (raw_parity > max_parity_shards) ? max_parity_shards : raw_parity;
    int data_shards = max_data_shards;
    int total_shards = data_shards + parity_shards;

    if (parity_shards <= 0) {
        fprintf(stderr, "Not enough room for parity shards. Reduce loss_rate or increase block_size.\n");
        return NULL;
    }

    unsigned char** data = malloc(sizeof(unsigned char*) * data_shards);
    for (int i = 0; i < data_shards; i++) {
        data[i] = malloc(block_size);
        int offset = i * block_size;
        int copy_size = (offset + block_size <= file_size) ? block_size : (file_size - offset);
        memcpy(data[i], file_data + offset, copy_size);
        if (copy_size < block_size) memset(data[i] + copy_size, 0, block_size - copy_size);
    }

    unsigned char** fec = malloc(sizeof(unsigned char*) * parity_shards);
    for (int i = 0; i < parity_shards; i++) {
        fec[i] = calloc(block_size, 1);
    }

    fec_init();
    reed_solomon* rs = reed_solomon_new(data_shards, parity_shards);
    if (!rs || reed_solomon_encode(rs, data, fec, block_size) != 0) {
        fprintf(stderr, "RS encode failed.\n");
        goto fail;
    }

    int encoded_size = sizeof(int) * 4 + total_shards * block_size;
    unsigned char* buffer = malloc(encoded_size);
    if (!buffer) goto fail;

    unsigned char* ptr = buffer;
    memcpy(ptr, &data_shards, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, &parity_shards, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, &block_size, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, &file_size, sizeof(int)); ptr += sizeof(int);

    for (int i = 0; i < data_shards; i++) {
        memcpy(ptr, data[i], block_size);
        ptr += block_size;
    }

    for (int i = 0; i < parity_shards; i++) {
        memcpy(ptr, fec[i], block_size);
        ptr += block_size;
    }

    for (int i = 0; i < data_shards; i++) free(data[i]);
    for (int i = 0; i < parity_shards; i++) free(fec[i]);
    free(data);
    free(fec);
    reed_solomon_release(rs);

    *out_encoded_size = encoded_size;
    return buffer;

fail:
    for (int i = 0; i < data_shards; i++) if (data[i]) free(data[i]);
    for (int i = 0; i < parity_shards; i++) if (fec[i]) free(fec[i]);
    free(data);
    free(fec);
    if (rs) reed_solomon_release(rs);
    return NULL;
}

// 主函数封装 I/O
int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: %s <input_file> <loss_rate_float> <block_size> <output_file>\n", argv[0]);
        return -1;
    }

    const char* input_file = argv[1];
    float loss_rate = atof(argv[2]);
    int block_size = atoi(argv[3]);
    const char* output_file = argv[4];

    if (block_size <= 0 || loss_rate <= 0.0 || loss_rate >= 1.0) {
        fprintf(stderr, "Invalid parameters. block_size must be > 0 and 0 < loss_rate < 1.\n");
        return -1;
    }

    // 读取原始文件内容
    FILE* fp_in = fopen(input_file, "rb");
    if (!fp_in) {
        perror("fopen input");
        return -1;
    }
    fseek(fp_in, 0, SEEK_END);
    int file_size = ftell(fp_in);
    rewind(fp_in);

    unsigned char* file_data = malloc(file_size);
    fread(file_data, 1, file_size, fp_in);
    fclose(fp_in);

    // 编码
    int encoded_size = 0;
    unsigned char* encoded_buf = encode_rs_from_buffer(file_data, file_size, loss_rate, block_size, &encoded_size);
    free(file_data);

    if (!encoded_buf) {
        fprintf(stderr, "Encoding failed.\n");
        return -1;
    }

    // 写入输出文件
    FILE* fp_out = fopen(output_file, "wb");
    if (!fp_out) {
        perror("fopen output");
        free(encoded_buf);
        return -1;
    }
    fwrite(encoded_buf, 1, encoded_size, fp_out);
    fclose(fp_out);
    free(encoded_buf);

    printf("Encoded and saved to: %s\n", output_file);
    return 0;
}
