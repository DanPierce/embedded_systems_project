#!/bin/bash

gcc host.c -o host -lpthread -lprussdrv

pasm -b sensor.p
