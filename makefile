vm: main.c  
	gcc $< -pthread -lm -o $@

.PHONY: run clean

run:
	./vm

clean:
	rm vm
