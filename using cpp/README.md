## This Assignement is done on linux (ubuntu)

## done by

-Harinadh sivaramakrishna 23CS06014 M.Tech
-shanmuka sharma 23CS06012 M.Tech
-Ritwik Raghav A23CS09001 P.h.D

## compiling command

- g++ -o lam1 lam1.cpp -lpthread -lboost_system -lboost_thread (lets say this for system 1)
- g++ -o lam2 lam2.cpp -lpthread -lboost_system -lboost_thread (lets say this for system 2)
- g++ -o lam3 lam3.cpp -lpthread -lboost_system -lboost_thread (lets say this for system 3)

## run command (here i have used loop back address to run on single system you can modify accordingly)

- layout is like ./filename current_system_id listeningPort_for_current_system system_id_2 ip port system_id_3 ip port
- ./lam1 1 6000 2 127.0.0.1 6001 3 127.0.0.1 6002
- ./lam2 2 6001 1 127.0.0.1 6000 3 127.0.0.1 6002
- ./lam3 3 6002 2 127.0.0.1 6001 1 127.0.0.1 6000
