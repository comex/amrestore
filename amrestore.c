#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef void *AMRestorableDeviceRef;
int AMRestorableDeviceRegisterForNotifications(void *eventHandler, void *arg, CFErrorRef *err);
uint64_t AMRestorableDeviceGetECID(AMRestorableDeviceRef device);
uint8_t AMRestorableDeviceRestore(AMRestorableDeviceRef device, int clientID, CFDictionaryRef options, void (*progressCallback)(AMRestorableDeviceRef device, CFDictionaryRef data, void *arg), void *a5, CFErrorRef *error);
void AMRestoreSetLogLevel(int level);
int AMRestoreEnableFileLogging(const char *filename);
void AMDSetLogLevel(int level);
void AMDAddLogFileDescriptor(int fd);
CFMutableDictionaryRef AMRestoreCreateDefaultOptions(CFAllocatorRef allocator);

static AMRestorableDeviceRef device;
static int clientID;
static const char *ipswDirectory;
static const char *signingServer = "http://127.0.0.1/";
static bool verbose;

#define cs(x) CFStringCreateWithCString(NULL, (x), kCFStringEncodingUTF8)

static void progressCallback(AMRestorableDeviceRef device, CFDictionaryRef data, void *arg) {
    if(verbose) {
        CFShow(data);
    }
    CFStringRef status = CFDictionaryGetValue(data, CFSTR("Status"));
    CFNumberRef progress = CFDictionaryGetValue(data, CFSTR("Progress"));
    CFNumberRef operation = CFDictionaryGetValue(data, CFSTR("Operation"));
    if(status) {
        if(progress && !verbose) {
            char cstatus[17];
            int cprogress;
            int coperation = 0;
            CFNumberGetValue(progress, kCFNumberIntType, &cprogress);
            if(operation) CFNumberGetValue(operation, kCFNumberIntType, &coperation);
            if(cprogress < 0) cprogress = 0;
            char pbar[50];
            CFStringGetCString(status, cstatus, sizeof(cstatus), kCFStringEncodingUTF8);
            for(int i = 0, j = 1; i < 50; i++, j += 2) {
                if(j < cprogress)
                    pbar[i] = '+';
                else if(j == cprogress)
                    pbar[i] = '-';
                else
                    pbar[i] = ' ';
            }

            fprintf(stderr, "\x1b[1A\x1b[K%14s   op %2d: %3d%% [%.50s]\n", cstatus, coperation, cprogress, pbar);
        }
        if(CFEqual(status, CFSTR("Successful"))) {
            exit(0);
        } else if(CFEqual(status, CFSTR("Failed"))) {
            CFErrorRef error = (void *) CFDictionaryGetValue(data, CFSTR("Error"));
            if(error) CFShow(error);
            exit(1);
        }
    }
}

static void eventHandler(AMRestorableDeviceRef _device, int type, void *arg) {
    device = _device;
    if(verbose) printf("eventHandler: type = %d\n", type);
    if(type == 0) { // connected
        uint64_t ecid = AMRestorableDeviceGetECID(device);
        printf("ECID: %016llx\n", ecid);
        CFMutableDictionaryRef options = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFMutableDictionaryRef bootOptions = AMRestoreCreateDefaultOptions(NULL);

        CFDictionarySetValue(bootOptions, CFSTR("AuthInstallVariant"), CFSTR("Customer Erase Install (IPSW)"));
        CFDictionarySetValue(bootOptions, CFSTR("AuthInstallSigningServerURL"), cs(signingServer));
        CFDictionarySetValue(bootOptions, CFSTR("SourceRestoreBundlePath"), cs(ipswDirectory));
        CFDictionarySetValue(options, CFSTR("BootOptions"), bootOptions);
        CFDictionarySetValue(options, CFSTR("RestoreOptions"), bootOptions);
        CFErrorRef error;
        int result = AMRestorableDeviceRestore(device, clientID, options, progressCallback, (void *) 0xdeadbeef, &error);
        if(!result) {
            fprintf(stderr, "couldn't restore\n");
            CFShow(error);
            exit(1);
        }
        fprintf(stderr, "...\n");
    }
}

int main(int argc, char **argv) {
    int c;
    while((c = getopt(argc, argv, "vs:")) != -1)
        switch(c) {
        case 's':
            signingServer = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        default:
            goto usage;
        }
    ipswDirectory = argv[optind++];
    if(!ipswDirectory || argv[optind]) goto usage;

    if(verbose) {
        AMDSetLogLevel(INT_MAX);
        AMDAddLogFileDescriptor(1);
        AMRestoreSetLogLevel(INT_MAX);
        AMRestoreEnableFileLogging("/dev/stderr");
    } else {
        AMDSetLogLevel(0);
        AMRestoreSetLogLevel(0);
    }
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
    printf("Usage: amrestore [-v] [-s url] dir\n"
           "  -s: signing server URL (default: http://127.0.0.1/)\n"
           "  -v: be verbose\n"
           " dir: directory with extracted IPSW contents\n");
    return 1;
}

