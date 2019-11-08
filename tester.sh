#!/bin/bash


echo "DEPARTURE TP440 init: 5 takeoff: 100" > Input_pipe
sleep 1;
echo "DEPARTURE TP441 init: 27 takeoff: 125" > Input_pipe
sleep 1;
echo "DEPARTURE TP442 init: 13 takeoff: 125" > Input_pipe
sleep 1;
echo "DEPARTURE TP6969 init: 13 " > Input_pipe
sleep 1;
echo "ARRIVAL TP42 init: 0 eta: 100 fuel: 1000" > Input_pipe
sleep 1;
echo "DEPARTURE TP4 init: 12 takeoff: 125" > Input_pipe
sleep 1;
echo "ARRIVAL TP12 init: 23 eta: 250 fuel: 500" > Input_pipe
sleep 1;
echo "Gibberish" > Input_pipe
sleep 1;
echo "ARRIVAL TP1221 init: 52 eta: 100 fuel: 2300" > Input_pipe
sleep 1;
echo "DEPARTURE TP123 init: 27 takeoff: 125" > Input_pipe
sleep 1;
echo "DEPARTURE TP422 init: 27 takeoff: 127" > Input_pipe


