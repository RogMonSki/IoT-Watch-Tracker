#!/bin/bash
# build.sh
# fire up iot:magic with . mapped as a volume and do a pio build

COMMAND="docker run "\
"-it --user 1000:1000 --group-add=dialout --group-add=ubuntu "\
"--volume=$(pwd):/home/ubuntu/project hamishcunningham/iot:magic "\
"pio run $*"

echo "$COMMAND"
eval $COMMAND
EXIT_STATUS=$?
BINARY="$(find . -name firmware_merged.bin)"
[ "x${BINARY}" != x ] && \
  echo -e "merged binary ready to flash at\n  ${BINARY}"
exit $EXIT_STATUS
