CFLAGS = -O2
LDFLAGS = -lcglm -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

default: clean compile run

clean:
	@rm -f ./bin/*
	@rm -f ./shaders/*.spv

compile:
	@./compile.sh
	@clang $(CFLAGS) -o ./bin/HelloTriangle $(wildcard *.c) $(LDFLAGS)

run:
	@./bin/HelloTriangle
