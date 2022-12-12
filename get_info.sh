#!/bin/bash

FILEPATH="/proc/lab/struct_info"

function show_help() {
    cat << END
Usage: ./usr.sh struct_id PID
Avaliable struct_id:
    0 - pt_regs
    1 - task_struct
END
}

if [[ $# != 2 ]]; then
    show_help
    exit
fi

echo "$1 $2" > $FILEPATH
cat $FILEPATH
