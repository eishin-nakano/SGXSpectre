#!/bin/bash

sum1=0
sum2=0
count=50

for i in $(seq 1 $count); do
	echo "Running $i..."

	lines=$(./sgxspectre 2 2 1 2>/dev/null | tail -n 2)

	val1=$(echo "$lines" | head -n 1)
	val2=$(echo "$lines" | tail -n 1)

	sum1=$(echo "$sum1 + $val1" | bc)
	sum2=$(echo "$sum2 + $val2" | bc)
done

avg1=$(echo "scale=6; $sum1 / $count" | bc)
avg2=$(echo "scale=6; $sum2 / $count" | bc)

echo "Acc ave: $avg1" >> ./result/result.txt
echo "time ave: $avg2" >> ./result/result.txt
