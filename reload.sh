#!/bin/bash

MODNAME="info-module"

make
sudo rmmod $MODNAME
sudo insmod ${MODNAME}.ko
