#!/bin/bash

$FBU $srcdir/return_leak.fbu 2>&1 | grep -q 'return leaks mutable pointers'