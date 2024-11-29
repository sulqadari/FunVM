#!/bin/sh

# check if an argument is an empty string or not.
if [ -z "$1" ]; then
	echo "ERROR."
	echo "The correct usage is: run.sh <source_name.fn>"
	exit 1
fi

script_name=$1
script_path=$(pwd)/tests/$script_name
binary_path=$(pwd)/tests/$script_name"b"
compiler_path=$(pwd)/build/compiler/FVMCexe
vm_path=$(pwd)/build/vm/FVMexe

$compiler_path  $script_path
$vm_path $binary_path