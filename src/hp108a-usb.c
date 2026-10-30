/* HP Laser 108a CUPS USB backend.
 * Adapted from paviladtudk/hp-laser-107a-macos hp107a-usb.c and
 * Kuberwastaken/hp-laser-1008a-macos (MIT; see vendor/LICENSE-usb-MIT).
 * Local changes: exact VID/PID, ambiguous-device refusal, streaming input,
 * no daemon/process termination/reset, no replay after partial transfer.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cups/backend.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

static void logmsg(const char *fmt, ...) {
    va_list ap; fputs("DEBUG: hp108a: ", stderr);
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fputc('\n', stderr);
}

/* Search a single registry class at a time to avoid counting aliases twice. */
static IOUSBDeviceInterface **find_device(void) {
    const char *classes[] = {"IOUSBHostDevice", kIOUSBDeviceClassName};
    for (int ci = 0; ci < 2; ci++) {
        io_iterator_t iter = 0;
        CFMutableDictionaryRef match = IOServiceMatching(classes[ci]);
        if (!match || IOServiceGetMatchingServices(kIOMainPortDefault, match, &iter) != KERN_SUCCESS) continue;
        IOUSBDeviceInterface **found = NULL;
        unsigned count = 0;
        io_service_t svc;
        while ((svc = IOIteratorNext(iter))) {
            IOCFPlugInInterface **plugin = NULL; SInt32 score = 0;
            IOUSBDeviceInterface **dev = NULL;
            if (IOCreatePlugInInterfaceForService(svc, kIOUSBDeviceUserClientTypeID,
                kIOCFPlugInInterfaceID, &plugin, &score) == KERN_SUCCESS && plugin) {
                (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID *)&dev);
                (*plugin)->Release(plugin);
            }
            IOObjectRelease(svc);
            if (!dev) continue;
            UInt16 vid = 0, pid = 0;
            (*dev)->GetDeviceVendor(dev, &vid); (*dev)->GetDeviceProduct(dev, &pid);
            if (vid == 0x03f0 && pid == 0xe82a) {
                count++;
                if (!found) found = dev; else (*dev)->Release(dev);
            } else (*dev)->Release(dev);
        }
        IOObjectRelease(iter);
        if (count > 1) {
            fprintf(stderr, "ERROR: Multiple HP 103/107/108 devices; connect only the target printer.\n");
            (*found)->Release(found); return NULL;
        }
        if (found) return found;
    }
    return NULL;
}

static int find_classic_iface(IOUSBDeviceInterface **dev, UInt8 *ifaceNum, UInt8 *alt) {
    IOUSBConfigurationDescriptorPtr cfg = NULL;
    if ((*dev)->GetConfigurationDescriptorPtr(dev, 0, &cfg) != kIOReturnSuccess || !cfg) return 0;
    const unsigned char *p = (const unsigned char *)cfg;
    int total = p[2] | (p[3] << 8);
    int curCls = -1, curProto = -1; UInt8 curIf = 0, curAlt = 0;
    for (int i = 0; i + 2 <= total; ) {
        int len = p[i], type = p[i + 1];
        if (len < 2 || i + len > total) break;
        if (type == 4 && len >= 9) {                         // interface descriptor
            curIf = p[i + 2]; curAlt = p[i + 3]; curCls = p[i + 5]; curProto = p[i + 7];
        } else if (type == 5 && len >= 7) {                  // endpoint descriptor
            int addr = p[i + 2], isBulk = (p[i + 3] & 3) == 2, isOut = !(addr & 0x80);
            if (curCls == 7 && (curProto == 1 || curProto == 2) && isBulk && isOut) {
                *ifaceNum = curIf; *alt = curAlt; return 1;        // first classic bulk-OUT wins
            }
        }
        i += len;
    }
    return 0;
}

// Scan the device's interfaces and seize the classic printer interface, returning it plus
// its bulk-OUT pipe. Sets *sawExclusive when a seize fails with kIOReturnExclusiveAccess
// (macOS's IPP-USB driver holding the interface), which the caller resolves by resetting the
// configuration. Matches the classic interface by NUMBER (its live nub may be on the IPP-USB
// alt) and forces the classic alt with SetAlternateInterface before finding the bulk-OUT.
static IOUSBInterfaceInterface **scan_and_seize(IOUSBDeviceInterface **dev, int haveTarget,
                                                UInt8 wantIf, UInt8 wantAlt, UInt8 *pipeOut,
                                                int *sawExclusive) {
    IOUSBFindInterfaceRequest req;
    req.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
    req.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    req.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    req.bAlternateSetting  = kIOUSBFindInterfaceDontCare;

    io_iterator_t iter = 0;
    if ((*dev)->CreateInterfaceIterator(dev, &req, &iter) != kIOReturnSuccess) return NULL;

    IOUSBInterfaceInterface **result = NULL;
    io_service_t usbIf;
    while ((usbIf = IOIteratorNext(iter))) {
        IOCFPlugInInterface **plugin = NULL; SInt32 score = 0;
        IOUSBInterfaceInterface **intf = NULL;
        if (IOCreatePlugInInterfaceForService(usbIf, kIOUSBInterfaceUserClientTypeID,
                kIOCFPlugInInterfaceID, &plugin, &score) == KERN_SUCCESS && plugin) {
            (*plugin)->QueryInterface(plugin,
                CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID *)&intf);
            (*plugin)->Release(plugin);
        }
        IOObjectRelease(usbIf);
        if (!intf) continue;

        UInt8 cls = 0, sub = 0, proto = 0, altNum = 0, ifNum = 0;
        (*intf)->GetInterfaceClass(intf, &cls);
        (*intf)->GetInterfaceSubClass(intf, &sub);
        (*intf)->GetInterfaceProtocol(intf, &proto);
        (*intf)->GetAlternateSetting(intf, &altNum);
        (*intf)->GetInterfaceNumber(intf, &ifNum);
        logmsg("  iface num=%u class=%u sub=%u proto=%u alt=%u", ifNum, cls, sub, proto, altNum);

        // Target the classic interface by number when we know it (its nub may be on the
        // IPP-USB alt right now); otherwise fall back to the old class/proto match.
        int isTarget = haveTarget ? (ifNum == wantIf) : (cls == 7 && (proto == 1 || proto == 2));
        if (isTarget) {
            IOReturn ir = (*intf)->USBInterfaceOpenSeize(intf);
            if (ir == kIOReturnSuccess) {
                UInt8 useAlt = haveTarget ? wantAlt : altNum;
                IOReturn ar = (*intf)->SetAlternateInterface(intf, useAlt);   // force classic (raw) alt
                logmsg("SetAlternateInterface(%u) -> 0x%08x", useAlt, ar);
                if (ar != kIOReturnSuccess) {
                    (*intf)->USBInterfaceClose(intf); (*intf)->Release(intf); continue;
                }
                UInt8 n = 0; (*intf)->GetNumEndpoints(intf, &n);
                for (UInt8 pipe = 1; pipe <= n; pipe++) {
                    UInt8 dir = 0, num = 0, tt = 0, interval = 0; UInt16 mps = 0;
                    (*intf)->GetPipeProperties(intf, pipe, &dir, &num, &tt, &mps, &interval);
                    if (dir == kUSBOut && tt == kUSBBulk) {
                        *pipeOut = pipe; result = intf;
                        logmsg("  -> using bulk-out pipe %u (ep 0x%02x) on iface %u alt %u", pipe, num, ifNum, useAlt);
                        break;
                    }
                }
                if (result) break;
                (*intf)->USBInterfaceClose(intf);
            } else {
                if (ir == kIOReturnExclusiveAccess) *sawExclusive = 1;
                logmsg("  seize failed 0x%08x%s", ir,
                       ir == kIOReturnExclusiveAccess ? " (interface held by macOS)" : "");
            }
        }
        (*intf)->Release(intf);
    }
    IOObjectRelease(iter);
    return result;
}


static int probe(void) {
    IOUSBDeviceInterface **dev = find_device();
    if (!dev) { fprintf(stderr, "HP 03f0:e82a not found or inaccessible.\n"); return 1; }
    UInt8 num = 0, alt = 0;
    int classic = find_classic_iface(dev, &num, &alt);
    printf("USB 03f0:e82a classic=%d interface=%u alternate=%u\n", classic, num, alt);
    (*dev)->Release(dev);
    return classic ? 0 : 1;
}

static int print_job(int fd) {
    IOUSBDeviceInterface **dev = find_device();
    if (!dev) {
        fprintf(stderr, "ERROR: HP Laser 108a USB device not available.\n");
        return CUPS_BACKEND_STOP;
    }
    int rc = CUPS_BACKEND_STOP;
    IOReturn opened = (*dev)->USBDeviceOpenSeize(dev);
    if (opened != kIOReturnSuccess) {
        fprintf(stderr, "ERROR: Cannot open HP USB device (0x%08x). Check IPP-USB bridge and VM ownership.\n", opened);
        (*dev)->Release(dev); return rc;
    }
    UInt8 num = 0, alt = 0, pipe = 0;
    int busy = 0;
    IOUSBInterfaceInterface **intf = NULL;
    if (find_classic_iface(dev, &num, &alt))
        intf = scan_and_seize(dev, 1, num, alt, &pipe, &busy);
    if (!intf) {
        fprintf(stderr, "ERROR: Cannot open classic printer interface (busy=%d).\n", busy);
        goto cleanup_device;
    }
    unsigned char buf[65536];
    size_t sent = 0;
    for (;;) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { perror("ERROR: read print job"); break; }
        if (n == 0) {
            if (sent) rc = CUPS_BACKEND_OK;
            else fprintf(stderr, "ERROR: Empty print job.\n");
            break;
        }
        IOReturn r = (*intf)->WritePipe(intf, pipe, buf, (UInt32)n);
        if (r != kIOReturnSuccess) {
            fprintf(stderr, "ERROR: USB write failed (0x%08x), %zu bytes previously sent. Job stopped; check printer before resuming.\n", r, sent);
            break;
        }
        sent += (size_t)n;
    }
    logmsg("USB bytes transferred: %zu; result=%d", sent, rc);
    (*intf)->USBInterfaceClose(intf); (*intf)->Release(intf);
cleanup_device:
    (*dev)->USBDeviceClose(dev); (*dev)->Release(dev);
    return rc;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--probe")) return probe();
    if (argc == 1) {
        IOUSBDeviceInterface **dev = find_device();
        if (dev) {
            puts("direct hp108a:/ \"HP Laser 108a\" \"HP Laser 108a (SPL USB)\" \"MFG:HP;MDL:HP Laser 103 107 108;CMD:SPL;\"");
            (*dev)->Release(dev);
        }
        return 0;
    }
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "Usage: hp108a job-id user title copies options [file] | --probe\n");
        return CUPS_BACKEND_FAILED;
    }
    /* The raster filter encodes copies into the page records. Do not repeat here. */
    int fd = STDIN_FILENO;
    if (argc == 7 && (fd = open(argv[6], O_RDONLY)) < 0) {
        perror("ERROR: open print job"); return CUPS_BACKEND_FAILED;
    }
    int rc = print_job(fd);
    if (fd != STDIN_FILENO) close(fd);
    return rc;
}
