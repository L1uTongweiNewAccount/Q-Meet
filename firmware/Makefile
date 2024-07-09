CFLAG := -mmcs51 --fomit-frame-pointer --xram-size 0x4000 --xram-loc 0x0000 --stack-auto --use-non-free
build:
	-@mkdir build
	sdcc $(CFLAG) main.c --out-fmt-ihx -o build/main
	packihx build/main > build/main.hex
emulator:
	-@mkdir build
	gcc -g -DEMULATOR -Wno-incompatible-pointer-types main.c -o build/emulator
clean:
	rm -rf build