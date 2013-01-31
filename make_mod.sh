MAP_NAME=$1
echo Creating mod file from ${MAP_NAME}.xml and ${MAP_NAME}_mod.xml
CRC32=`crc32 data/maps/${MAP_NAME}.xml`
echo CRC32 ${CRC32} > data/maps/${MAP_NAME}.mod
diff -e data/maps/${MAP_NAME}.xml data/maps/${MAP_NAME}_mod.xml >> data/maps/${MAP_NAME}.mod
