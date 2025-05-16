gcc -std=c99 encode_demo.c rs.c -o encode
gcc -std=c99 drop.c -o drop
gcc -g -std=c99 decode_demo.c rs.c -o decode
./encode 1.avp 0.2 192 enc.bin   #在1.avp的基础上加上校验码，生成enc.bin
./drop enc.bin drop.bin 2 5   #让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
./decode drop.bin rec.avp    #解码恢复

#./encode 1.avp 0.3 300 enc.bin   #在1.avp的基础上加上校验码，生成enc.bin
#./drop enc.bin 2 5   #让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
#./decode out.bin rec.avp    #解码恢复
#./encode 1.avp 0.4 300 enc.bin   #在1.avp的基础上加上校验码，生成enc.bin
#./drop enc.bin 2 5   #让enc.bin模拟卫星丢包，填192个00, 并生成out.bin
#./decode out.bin rec.avp    #解码恢复


