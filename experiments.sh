#!/bin/bash

#Runs and stores time taken for matrices of sizes 1000, 1500, and 2000, using
#1-16 threads.

gcc -Wall -o orig mm.c
gcc -Wall -o new pt-mm.c -pthread
tottime=0
echo "Averages for dimension of 1500 multiplication" > averages
for i in {1..16..1}; do
  for j in {0..4..1}; do
    if [ $i -eq 1 ]
    then
      time=$(./orig -T -x1500 -y1500 -z1500 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    else
      time=$(./new -T -n$i -x1500 -y1500 -z1500 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    fi
  done
  #Take average
  echo $tottime/5 | bc -l >> averages
  tottime=0
done
echo "Averages for dimension of 1750 multiplication" >> averages
for i in {1..16..1}; do
  for j in {0..4..1}; do
    if [ $i -eq 1 ]
    then
      time=$(./orig -T -x1750 -y1750 -z1750 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    else
      time=$(./new -T -n$i -x1750 -y1750 -z1750 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    fi
  done
  #Take average
  echo $tottime/5 | bc -l >> averages
  tottime=0
done
echo "Averages for dimension of 2000 multiplication" >> averages
for i in {1..16..1}; do
  for j in {0..4..1}; do
    if [ $i -eq 1 ]
    then
      time=$(./orig -T -x2000 -y2000 -z2000 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    else
      time=$(./new -T -n$i -x2000 -y2000 -z2000 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    fi
  done
  #Take average
  echo $tottime/5 | bc -l >> averages
  tottime=0
done
echo "Averages for dimension of 1000 ^6" >> averages
for i in {1..16..1}; do
  for j in {0..4..1}; do
    if [ $i -eq 1 ]
    then
      time=$(./orig -T -s6 -x1000  | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    else
      time=$(./new -T -s6 -n$i -x1000 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    fi
  done
  #Take average
  echo $tottime/5 | bc -l >> averages
  tottime=0
done
echo "Averages for dimension of 1150 ^6" >> averages
for i in {1..16..1}; do
  for j in {0..4..1}; do
    if [ $i -eq 1 ]
    then
      time=$(./orig -T -s6 -x1150 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    else
      time=$(./new -T -s6 -n$i -x1150 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    fi
  done
  #Take average
  echo $tottime/5 | bc -l >> averages
  tottime=0
done
echo "Averages for dimension of 1300 ^6" >> averages
for i in {1..16..1}; do
  for j in {0..4..1}; do
    if [ $i -eq 1 ]
    then
      time=$(./orig -T -s6 -x1300 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    else
      time=$(./new -T -s6 -n$i -x1300 | awk -F ',| ' '{print $4}')
      tottime=$((tottime+time))
    fi
  done
  #Take average
  echo $tottime/5 | bc -l >> averages
  tottime=0
done

#Calculate speed ups
for i in {0..5..1}; do
  #skip over section descriptors
  read skip
  read origtime
  for j in {1..15..1}; do
    read newtime
    echo $origtime/$newtime | bc -l >> speedups
  done
done < averages
