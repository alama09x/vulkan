CFLAGS = -O2
LDFLAGS = -lcglm -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

default: clean compile run

clean:
	@rm -f ./bin/HelloTriangle

compile:
	@clang $(CFLAGS) -o ./bin/HelloTriangle $(wildcard *.c) $(LDFLAGS)

run:
	@./bin/HelloTriangle
