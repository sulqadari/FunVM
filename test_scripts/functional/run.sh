# Kludge which is intended to simplify test cases running procedure.
cd ./build/test_scripts/functional

echo "******************************************************"
echo "************ RUNNING FUNCTIONAL TESTS ****************"
echo "******************************************************"

echo ""
echo "*** TEST: EXPRESSIONS ***"
./../../FunVM test_expressions.fn
if [ "$?" -ne 0 ]; then
	echo "Tests execution have interrupted."
	exit 1
fi

echo ""
echo "*** TEST: GLOBAL VARIABLES ***"
./../../FunVM test_globalVariables.fn
if [ "$?" -ne 0 ]; then
	echo "Tests execution have interrupted."
	exit 1
fi

echo ""
echo "*** TEST: LOCAL VARIABLES ***"
./../../FunVM test_localVariables.fn
if [ "$?" -ne 0 ]; then
	echo "Tests execution have interrupted."
	exit 1
fi

echo ""
echo "*** TEST: STRINGS ***"
./../../FunVM test_strings.fn
if [ "$?" -ne 0 ]; then
	echo "Tests execution have interrupted."
	exit 1
fi
echo "******************************************************"
echo "*** FUNCTIONAL TESTS HAVE BEEN PASSED SUCCESSFULLY ***"
echo "******************************************************"
cd ../../../