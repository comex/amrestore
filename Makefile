amrestore: amrestore.c
	gcc -o $@ $< -framework MobileDevice -F/System/Library/PrivateFrameworks -framework CoreFoundation
clean:
	rm -f amrestore
