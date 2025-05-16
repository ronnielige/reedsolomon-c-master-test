gcc -g -std=c99 encode_demo.c rs.c -o     bin/encode
gcc -std=c99        drop.c      -o     bin/drop
gcc -g -std=c99 decode_demo.c rs.c -o  bin/decode
bin/encode  bin/1.avp   10 2  192  bin/enc.bin       # 在1.avp的基础上加上校验码，生成enc.bin
bin/drop    bin/enc.bin  bin/drop.bin 192  2  5      # 让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
bin/decode  bin/drop.bin bin/rec.avp                 # 解码恢复

#./encode 1.avp 0.3 300 enc.bin   #在1.avp的基础上加上校验码，生成enc.bin
#./drop enc.bin 2 5   #让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
#./decode out.bin rec.avp    #解码恢复
#./encode 1.avp 0.4 300 enc.bin   #在1.avp的基础上加上校验码，生成enc.bin
#./drop enc.bin 2 5   #让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
#./decode out.bin rec.avp    #解码恢复


