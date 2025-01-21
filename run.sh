#!/bin/sh

# check if an argument is an empty string or not.
if [ -z "$1" ]; then
	echo "ERROR."
	echo "The correct usage is: run.sh <source_name.fn>"
	exit 1
fi


script_name=$1

source_path=$(pwd)/tests/
output_path=$(pwd)/tests/bin/
compiler_path=$(pwd)/build/bin/FVMCexe
vm_path=$(pwd)/build/bin/FVMexe

echo "********************************"
echo "*       FunVM compiler         *"
echo "********************************"

set -e #halt further execution if compiler returns a non-zero value
$compiler_path  $script_name $source_path $output_path

echo "********************************"
echo "*      FunVM interpreter       *"
echo "********************************"
$vm_path $output_path$script_name"b"