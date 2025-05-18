#ifndef __RS_H_
#define __RS_H_

/* use small value to save memory */
#ifndef DATA_SHARDS_MAX
#define DATA_SHARDS_MAX (255)
#endif

/* use other memory allocator */
#ifndef RS_MALLOC
#define RS_MALLOC(x)    malloc(x)
#endif

#ifndef RS_FREE
#define RS_FREE(x)      free(x)
#endif

#ifndef RS_CALLOC
#define RS_CALLOC(n, x) calloc(n, x)
#endif

typedef struct _reed_solomon {
    int data_shards;
    int parity_shards;
    int shards;
    unsigned char* m;
    unsigned char* parity;
} reed_solomon;

// define 8 bytes prefix content:  data_shards (1 char) + parity_shards (1 char) + block_size (1 short) + file_size ( 1 int)
#define PREFIX_BYTES 8

typedef struct _reed_solomon_handle {
    int data_shards;             // data shards
    int parity_shards;           // parity shards
    int prefix_size;             // size of prefix data added at the beginning of each packet 
    int block_size;              // size of packet
    int block_data_size;         // size of data in each packet exclude prefix data
    int total_size;              // total processed buffer size
    int remain_size;
    unsigned char** data;        // data[i] point to i-th data_shards
    unsigned char*  data_buffer; // buffer to store original data and fec data(fec data append at the end)
    unsigned char** fec;         // fec[i] point to i-th parity_shards
    reed_solomon* rs;
} reed_solomon_handle;

/**
 * MUST initial one time
 * */
void fec_init(void);

reed_solomon* reed_solomon_new(int data_shards, int parity_shards);
void reed_solomon_release(reed_solomon* rs);

/**
 * encode one shard
 * input:
 * rs
 * data_blocks[rs->data_shards][block_size]
 * fec_blocks[rs->data_shards][block_size]
 * */
int reed_solomon_encode(reed_solomon* rs,
        unsigned char** data_blocks,
        unsigned char** fec_blocks,
        int block_size);


/**
 * decode one shard
 * input:
 * rs
 * original data_blocks[rs->data_shards][block_size]
 * dec_fec_blocks[nr_fec_blocks][block_size]
 * fec_block_nos: fec pos number in original fec_blocks
 * erased_blocks: erased blocks in original data_blocks
 * nr_fec_blocks: the number of erased blocks
 * */
int reed_solomon_decode(reed_solomon* rs,
        unsigned char **data_blocks,
        int block_size,
        unsigned char **dec_fec_blocks,
        unsigned int *fec_block_nos,
        unsigned int *erased_blocks,
        int nr_fec_blocks);

/**
 * encode a big size of buffer
 * input:
 * rs
 * nr_shards: assert(0 == nr_shards % rs->data_shards)
 * shards[nr_shards][block_size]
 * */
int reed_solomon_encode2(reed_solomon* rs, unsigned char** shards, int nr_shards, int block_size);

/**
 * reconstruct a big size of buffer
 * input:
 * rs
 * nr_shards: assert(0 == nr_shards % rs->data_shards)
 * shards[nr_shards][block_size]
 * marks[nr_shards] marks as errors
 * */
int reed_solomon_reconstruct(reed_solomon* rs, unsigned char** shards, unsigned char* marks, int nr_shards, int block_size);



reed_solomon_handle* reed_solomon_handle_new(int data_shards, int parity_shards, int prefix_size, int block_size, int total_size);

void reed_solomon_handle_release(reed_solomon_handle* h);
#endif

