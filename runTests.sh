#!/bin/sh
echo "************ RUNNING FUNCTIONAL TESTS ****************"
echo " "


for currScript in ./tests/functional/*.fn
do
	echo "*** $currScript ***"
	./build/FunVM $currScript
	if [ "$?" -ne 0 ]; then
		echo "Tests execution have been interrupted."
		exit 1
	fi
	echo "PASSED"
	echo " "
done

echo " "
echo "*** FUNCTIONAL TESTS HAVE BEEN PASSED SUCCESSFULLY ***"
echo " "
