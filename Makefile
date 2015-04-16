all: build

build: cpu_convert cpu_convert_sse

cpu_convert: cpu_convert.c
	gcc -Ofast -march=native -std=gnu99 cpu_convert.c -o cpu_convert
	
cpu_convert_sse: cpu_convert_sse.c
	gcc -Ofast -march=native -std=gnu99 cpu_convert_sse.c -o cpu_convert_sse

run: build
	$(EXEC) ./cpu_convert

clean:
	rm -f cpu_convert cpu_convert_sse

clobber: clean
