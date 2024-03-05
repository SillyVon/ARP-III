.PHONY: all clean directories server_new main motion watchdog target obstacle run

all: clean directories server_new main motion watchdog target obstacle


clean:
	@rm -rf bin
	@rm -rf Logs

directories:
	mkdir bin
	mkdir Logs

refresh:
	@rm -rf Logs
	@mkdir Logs

server_new:
	gcc src/server_.c -o bin/server_new -lm -lncurses 

main: 
	gcc src/main.c -o bin/main -lm -lncurses

motion:
	gcc src/motion_dynamics.c -o bin/motion -lm -lncurses

watchdog:
	gcc src/watchdog.c -o bin/watchdog -lm -lncurses

target:
	gcc src/target_numbers.c -o bin/target -lm -lncurses

obstacle:
	gcc src/obstacles_spawner.c -o bin/obstacle -lm -lncurses

run: 
	./bin/server_new


