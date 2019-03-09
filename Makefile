all: compile run

compile:
	@mkdir -p build &&\
	cd build &&\
	echo "\n\033[1;33m█ Generating makefiles...\033[0m\n" &&\
	cmake .. &&\
	echo "\n\033[1;93m█ Compiling...\033[0m\n" &&\
	make

run:
	echo "\n\033[1;32m█ Running...\033[0m\n\n" &&\
	./build/ZED_GroundMapper
