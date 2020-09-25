#!/bin/sh
binary="./schedulersim"

validate_test() {
    binary=$1
    trace_name=$2
    config=$3

    config_name=`echo $config | tr -d '-' | tr ' ' '_'`

    ${binary} ${config} < test-traces/${trace_name}.trc > myoutput/${trace_name}_${config_name}.out
    diff output/${trace_name}_${config_name}.out myoutput/${trace_name}_${config_name}.out
}

if [ ! -f "${binary}" ]
then
    echo "Executable ${binary}" not found
    exit 1
fi

rm -rf myoutput
mkdir myoutput

for trace_name in "all-independent" "complete-chain" "multi-fire"
do
    validate_test "${binary}" "$trace_name" "-u6"
    validate_test "${binary}" "$trace_name" "-a2 -d2 -m2"
done
