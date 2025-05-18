#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs.h"

// 封装编码逻辑的函数
unsigned char* encode_rs_from_buffer( reed_solomon_handle* h, const unsigned char* data, int data_size, int* out_encoded_size)
{
    if (h->block_size <= 0 || data_size <= 0) {
        fprintf(stderr, "Invalid parameters.\n");
        return NULL;
    }
    int total_shards = h->data_shards + h->parity_shards;

    if (h->parity_shards <= 0) {
        fprintf(stderr, "Not enough room for parity shards. Reduce loss_rate or increase block_size.\n");
        return NULL;
    }

    memset(h->data_buffer, 0, h->block_size * (h->data_shards + h->parity_shards));  // 一开始全部置为0
    for (int i = 0; i < h->data_shards; i++)  // 拷贝数据到data_shards中
    {
        int offset = i * h->block_data_size;
        int copy_size = (offset + h->block_data_size <= data_size) ? h->block_data_size : (data_size - offset);
        if(copy_size > 0)
            memcpy(h->data[i] + h->prefix_size, data + offset, copy_size);

        // 每一个packet增加 8 字节前置数据
        unsigned char *ptr = h->data[i];
        memcpy(ptr, &h->data_shards, sizeof(unsigned char));
        ptr += sizeof(unsigned char);
        memcpy(ptr, &h->parity_shards, sizeof(unsigned char));
        ptr += sizeof(unsigned char);
        memcpy(ptr, &h->block_size, sizeof(short));
        ptr += sizeof(short);
        memcpy(ptr, &h->total_size, sizeof(int));
        ptr += sizeof(int);
    }

    if (!h->rs || reed_solomon_encode(h->rs, h->data, h->fec, h->block_size) != 0) {
        fprintf(stderr, "RS encode failed.\n");
        goto fail;
    }

    *out_encoded_size = (h->data_shards + h->parity_shards) * h->block_size;
    return h->data_buffer;

fail:
    return NULL;
}

// 主函数封装 I/O
int main(int argc, char* argv[]) {
    if (argc != 6) {
        printf("Usage:   %s <input_file> <data_shards> <parity_shards> <block_size> <output_file>\n", argv[0]);
        printf("example: %s input.bin  10  2  192 output.bin   \n", argv[0]);
        printf("         Means packet size = 192 bytes, 10 data packets generate 2 fec packets\n");
        printf("Note:    data_shards and parity_shards must < 255, and data_shards + parity_shards must < 255 !!!\n\n");
        return -1;
    }
    const char* input_file = argv[1];
    int data_shards   = atof(argv[2]);  // should < 255
    int parity_shards = atof(argv[3]);  // should < 255 and data_shards + parity_shards must < 255
    int block_size    = atoi(argv[4]);  // satilite transport packet size = 192
    const char* output_file = argv[5];

    if (block_size <= 0) {
        fprintf(stderr, "Invalid parameters. block_size must be > 0\n");
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
    unsigned char* p_data = file_data;
    fread(file_data, 1, file_size, fp_in);
    fclose(fp_in);

    FILE *fp_out = fopen(output_file, "wb"); // 追加模式
    if (!fp_out)
    {
        perror("fopen output");
        return -1;
    }

    // FEC编码流程
    int encoded_size = 0;
    int processed_size = 0;
    int data_size_once;
    reed_solomon_handle* h = reed_solomon_handle_new(data_shards, parity_shards, PREFIX_BYTES, block_size, file_size);  // 每一个packet增加 8 字节前置数据
    while (file_size > processed_size) // 分段处理输入文件，一次处理data_shards个数据块
    {
        int process_size_once = data_shards * h->block_data_size;
        data_size_once = file_size - processed_size > process_size_once ? process_size_once : file_size - processed_size;
        unsigned char *encoded_buf = encode_rs_from_buffer(h, p_data, data_size_once, &encoded_size);
        if (!encoded_buf)
        {
            fprintf(stderr, "Encoding failed.\n");
            return -1;
        }

        // 写入输出文件
        fwrite(encoded_buf, 1, encoded_size, fp_out);
        p_data         += data_size_once;
        processed_size += data_size_once;
    }

    free(file_data);
    fclose(fp_out);
    reed_solomon_handle_release(h);

    printf("Encoded and saved to: %s\n", output_file);
    return 0;
}
