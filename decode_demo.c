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

// 从 buffer 中搜索 主要参数
int search_rs_for_parameters(const unsigned char* encoded_buf, int encoded_size, int* data_shards, 
                             int* parity_shards, int* block_size, int* file_size)
{
    int idx;
    const int *ptr = (int *)encoded_buf;
    for(idx = 0; idx < encoded_size; idx++)
    {
        if(encoded_buf[idx] != 0)
        {
            *data_shards   = encoded_buf[idx];                // unsigned char
            *parity_shards = encoded_buf[idx + 1];            // unsigned char
            memcpy(block_size, &encoded_buf[idx + 2], sizeof(short));
            memcpy(file_size,  &encoded_buf[idx + 4], sizeof(int));
            return 1;
        }
    }
    return 0;
}

unsigned char* decode_rs_from_buffer(reed_solomon_handle* h, const unsigned char* encoded_buf, int encoded_size, int* out_file_size)
{
    if (!encoded_buf) {
        fprintf(stderr, "Invalid buffer.\n");
        return NULL;
    }

    const unsigned char* ptr = encoded_buf;
    int total_shards = h->data_shards + h->parity_shards;

    if (encoded_size < total_shards * h->block_size) {
        fprintf(stderr, "Encoded buffer too small.\n");
        return NULL;
    }

    // 分配 shard 指针和标记
    // unsigned char** shards = malloc(sizeof(unsigned char*) * total_shards);
    unsigned char* marks = calloc(total_shards, 1);  // 0=ok, 1=lost

    // 读取所有 shard
    int available_shards = total_shards;
    for (int i = 0; i < total_shards; i++) {
        memcpy(h->data[i], ptr, h->block_size);
        ptr += h->block_size;

        if (is_zero_block(h->data[i], h->block_size)) {
            available_shards--;
            marks[i] = 1;
        }
    }

    // 检查是否可恢复
    if (available_shards < h->data_shards) {
        fprintf(stderr, "ERROR: Not enough shards to recover. Needed %d, got %d\n", h->data_shards, available_shards);
        //goto fail;
    }

    // 解码
    if (!h->rs || reed_solomon_reconstruct(h->rs, h->data, marks, total_shards, h->block_size) != 0) {
        fprintf(stderr, "Reconstruction failed.\n");
        goto fail;
    }

    // 提取原始数据
    unsigned char* out_data = malloc(encoded_size);
    int copied = 0;
    for (int i = 0; i < h->data_shards && copied < h->total_size && h->remain_size > 0; i++) {
        int copy_len = (h->remain_size < h->block_size - h->prefix_size) ? h->remain_size: h->block_size - h->prefix_size;
        memcpy(out_data + copied, h->data[i] + h->prefix_size, copy_len);
        copied += copy_len;
        h->remain_size = h->remain_size - copy_len;
        // printf("copy_len = %d total_size = %d\n", copy_len, h->total_size);
    }

    // 清理
    free(marks);
    *out_file_size = copied;
    return out_data;

fail:
    free(marks);
    return NULL;
}

// 用法示例 main()
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_bin> <output_file>\n", argv[0]);
        return -1;
    }

    const char*  input_file = argv[1];
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

    FILE *fout = fopen(output_file, "wb");
    // First, search for parameters
    int data_shards; 
    int parity_shards; 
    int block_size; 
    int file_size;
    int ret;
    ret = search_rs_for_parameters(buf, buf_size, &data_shards, &parity_shards, &block_size, &file_size);
    printf("data_shards: %d, parity_shards: %d, block_size: %d, file_size: %d\n", data_shards, parity_shards, block_size, file_size);

    // Second, loop decode
    reed_solomon_handle* h = reed_solomon_handle_new(data_shards, parity_shards, PREFIX_BYTES, block_size, file_size);
    int processed_size = 0;
    unsigned char *p_data = buf;
    while(buf_size > processed_size)
    {
        int processed_size_once = (data_shards + parity_shards) * block_size; 
        int data_size_once = buf_size - processed_size > processed_size_once ? processed_size_once : buf_size - processed_size;
        int decoded_size = 0;
        unsigned char *decoded = decode_rs_from_buffer(h, p_data, data_size_once, &decoded_size);
        if (!decoded)
        {
            fprintf(stderr, "Decoding failed.\n");
            return -1;
        }
        fwrite(decoded, 1, decoded_size, fout);
        free(decoded);
        p_data         += data_size_once;
        processed_size += processed_size_once;
    }
    reed_solomon_handle_release(h);
    free(buf);
    fclose(fout);

    printf("Decoded and saved to: %s\n", output_file);
    return 0;
}
