qmeet-cli:
	-@mkdir build
	gcc -fomit-frame-pointer -DEMULATOR micro-ecc/uECC.c ../libs/sha-2/sha-256.c ../libs/argparse/argparse.c \
		../libs/base64/base64.c qmeet-cli.c -o build/qmeet-cli
clean:
	rm -rf build