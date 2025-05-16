# 处理1.avp
gcc -g -std=c99 encode_demo.c rs.c -o  bin/encode
gcc -std=c99        drop.c      -o     bin/drop
gcc -g -std=c99 decode_demo.c rs.c -o  bin/decode
bin/encode  bin/1.avp   10 2  192  bin/enc.bin       # 在1.avp的基础上加上校验码，生成enc.bin
bin/drop    bin/enc.bin  bin/drop.bin 192  2  5      # 让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
bin/decode  bin/drop.bin bin/rec.avp                 # 解码恢复

# 处理2.jpg
bin/encode  bin/2.jpg   50  5  192  bin/enc2.bin       # 在2.jpg的基础上加上校验码，生成enc.bin
bin/drop    bin/enc2.bin  bin/drop2.bin  192  2  5  55 62 75 153 169 189 256 289     # 让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
bin/decode  bin/drop2.bin bin/rec2.jpg                # 解码恢复


