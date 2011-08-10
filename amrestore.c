#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef void *AMRestorableDeviceRef;
int AMRestorableDeviceRegisterForNotifications(void *eventHandler, void *arg, CFErrorRef *err);
uint64_t AMRestorableDeviceGetECID(AMRestorableDeviceRef device);
uint8_t AMRestorableDeviceRestore(AMRestorableDeviceRef device, int clientID, CFDictionaryRef options, void *a4, void *a5, void *a6);
void AMRestoreSetLogLevel(int level);
void AMDSetLogLevel(int level);
void AMDAddLogFileDescriptor(int fd);
CFMutableDictionaryRef AMRestoreCreateDefaultOptions(CFAllocatorRef allocator);

static AMRestorableDeviceRef device;
static int clientID;
static const char *ipswDirectory;
static const char *signingServer = "http://127.0.0.1/";

#define cs(x) CFStringCreateWithCString(NULL, (x), kCFStringEncodingUTF8)

static void connectedCallback(CFRunLoopTimerRef timer, void *nul) {
    CFMutableDictionaryRef options = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef bootOptions = AMRestoreCreateDefaultOptions(NULL);

    CFDictionarySetValue(bootOptions, CFSTR("AuthInstallVariant"), CFSTR("Customer Erase Install (IPSW)"));
    CFDictionarySetValue(bootOptions, CFSTR("AuthInstallSigningServerURL"), cs(signingServer));
    CFDictionarySetValue(bootOptions, CFSTR("SourceRestoreBundlePath"), cs(ipswDirectory));
    CFDictionarySetValue(options, CFSTR("BootOptions"), bootOptions);
    CFDictionarySetValue(options, CFSTR("RestoreOptions"), bootOptions);
    int result = AMRestorableDeviceRestore(device, clientID, options, NULL, NULL, NULL);
    printf("result = %d\n", result);

}

static void eventHandler(AMRestorableDeviceRef _device, int type, void *arg) {
    device = _device;
    printf("eventHandler: type = %d\n", type);
    if(type == 0) { // connected
        uint64_t ecid = AMRestorableDeviceGetECID(device);
        printf("ECID: %016llx\n", ecid);
        CFRunLoopAddTimer(CFRunLoopGetMain(), CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), 0, 0, 0, connectedCallback, NULL), kCFRunLoopCommonModes);
    }
}

static void usage() {
}

int main(int argc, char **argv) {
    int c;
    while((c = getopt(argc, argv, "s:")) != -1)
        switch(c) {
        case 's':
            signingServer = optarg;
            break;
        default:
            goto usage;
        }
    ipswDirectory = argv[optind++];
    if(!ipswDirectory || argv[optind]) goto usage;

    AMDSetLogLevel(INT_MAX);
    AMRestoreSetLogLevel(INT_MAX);
    AMDAddLogFileDescriptor(1);
    CFErrorRef err;
    clientID = AMRestorableDeviceRegisterForNotifications(eventHandler, (void *) 0xdeadbeef, &err);
    if(!clientID) {
        fprintf(stderr, "couldn't register for notifications\n");
        CFShow(err);
        return 1;
    }
    CFRunLoopRun();
    return 0;
    
    usage:
    printf("Usage: amrestore [-s url] dir\n"
           "  -s: signing server URL (default: http://127.0.0.1/)\n"
           " dir: directory with extracted IPSW contents\n");
    return 1;
}

