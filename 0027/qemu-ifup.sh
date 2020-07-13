#!/bin/sh

brctl stp virbr0 off
brctl addif virbr0 $1
ifconfig $1 up
