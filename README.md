# ExactMVA_OpenMP
Multi-threaded implementation of Exact Mean Value Analysis algorithm exploiting OpenMP

For this first release, the application first computes MVA with a simple single-threaded C++ algorithm on the CPU. Then makes again computations using an OpenMP accelerated algorithm and compare execution times and results.

How to compile: Simply use command "make". It's only required to have installed an implementation of OpenMP. Linux systems usually come with a version preinstalled.

How to launch application: Type command "./mva" followed by one or more of the following options: 
-d [DEMANDS_FILE_PATH] 
-n [number of jobs] 
-z [think time] 
-k [number of stations] -> can be used only if "-d" option is not already used, in order to specify how many stations have to be considered. Demands of those stations are generated randomly.

Example: "./mva -d demands.txt -n 200 -z 1.5" 
If you do not have a demands file as source and you eant to try random demands, use: "./mva -k 350 -n 5000 -z 0.5"

Demands file has to be a text file containing demands values separated with comma, without spaces. 
You can look at the pre-laded file "./demands.txt" in the folder as an example. 
The number of stations is computed automaticcaly from the file, it is not required to be specified by "-k" attribute.
