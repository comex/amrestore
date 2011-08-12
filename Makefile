amrestore: amrestore.c
	gcc -o $@ $< -std=gnu99 -framework MobileDevice -F/System/Library/PrivateFrameworks -framework CoreFoundation
clean:
	rm -f amrestore
