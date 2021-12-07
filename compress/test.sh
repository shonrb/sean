#!/bin/bash
if [ $# -eq 0 ]
then
    echo "Filename argument expected"
    exit 1
fi
SOURCE=$1
SOURCE_COMP="${SOURCE}.huff"
COPY="${SOURCE}copy"
COPY_COMP="${COPY}.huff"

# Compression and decompression
./prog $SOURCE
cp $SOURCE_COMP $COPY_COMP
./prog $COPY_COMP

# Comparison
diff $SOURCE $COPY -s

# Cleanup
rm $SOURCE_COMP $COPY_COMP $COPY
