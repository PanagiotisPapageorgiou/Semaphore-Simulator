all: Simulator Generator Manager

Simulator: simulator.c simulator_tools.c shared_structs.c
	gcc -o Simulator simulator.c simulator_tools.c shared_structs.c -lm

Generator: generator.c generator_tools.c shared_structs.c
	gcc -o Generator generator.c generator_tools.c shared_structs.c -lm

Manager: manager.c manager_tools.c memory.c shared_structs.c
	gcc -o Manager manager.c manager_tools.c memory.c shared_structs.c -lm
