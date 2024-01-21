CFLAGS = -O2
LDFLAGS = -lcglm -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.c
	clang $(CFLAGS) -o VulkanTest main.c $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest
