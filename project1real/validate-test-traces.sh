#!/bin/bash
binary="./branchsim"

function validate_test() {
    binary=$1
    trace_name=$2
    p=$3
    k=$4
    c=$5
    s=$6

    config="-p${p} -k${k} -c${c} -s${s}"
    config_name=${trace_name}_${p}_${k}_${c}_${s}

    ${binary} ${config} < test-traces/${trace_name}.trc > myoutput/${config_name}.out
    diff test-output/${config_name}.out myoutput/${config_name}.out
}

if [ ! -f "${binary}" ]
then
    echo "Executable ${binary}" not found
    exit 1
fi

rm -rf myoutput
mkdir myoutput

for trace_name in "simple_for" "two_for" "biased_if" "even_odd_if"
do
    validate_test "${binary}" "$trace_name" B 16 2 4
    validate_test "${binary}" "$trace_name" G 16 2 4
    validate_test "${binary}" "$trace_name" L 16 2 3
    validate_test "${binary}" "$trace_name" T 16 2 3

    validate_test "${binary}" "$trace_name" B 64 1 4
    validate_test "${binary}" "$trace_name" G 64 1 6
    validate_test "${binary}" "$trace_name" L 64 1 4
    validate_test "${binary}" "$trace_name" T 64 1 4

    validate_test "${binary}" "$trace_name" B 64 2 2
    validate_test "${binary}" "$trace_name" G 64 2 6
    validate_test "${binary}" "$trace_name" L 64 2 4
    validate_test "${binary}" "$trace_name" T 64 2 4
done
