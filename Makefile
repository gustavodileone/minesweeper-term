all:
	gcc -Wall *.c -o ./build/minesweeper

rm:
	rm -R ./build/*