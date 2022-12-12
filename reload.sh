#!/bin/bash

MODNAME="lab-module"

make
sudo rmmod $MODNAME
sudo insmod ${MODNAME}.ko
