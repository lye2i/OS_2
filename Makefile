all: ssu_shell pps


ssu_shell:
	gcc -o ssu_shell ssu_shell.c

pps:
	gcc -o pps pps.c

clean:
	rm pps ssu_shell
