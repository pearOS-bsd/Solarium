// BSD-2-Clause. glass-probe: shows the macOS 26 Liquid Glass (NSGlassEffectView) over a known test
// background, saves a screenshot of every scene and dumps the Core Animation layer tree of the glass
// (class, filters with all their input values, SDF effects with their properties).
// No SDK dependency on the glass API: classes and properties are used by name at run time.
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

static const int W = 512, H = 512;
static NSString *gOut;

static NSImage *TestBackground(void) {   // the same pixels as Solarium/tools/vk-run
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:W pixelsHigh:H
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:W*4 bitsPerPixel:32];
    unsigned char *d = rep.bitmapData;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        unsigned char *p = d + (y*W + x)*4; int gx = (x%64 < 3), gy = (y%64 < 3);
        p[0] = 255*x/W; p[1] = 255*y/H; p[2] = 255 - 255*x/W; p[3] = 255;
        if (gx || gy) p[0] = p[1] = p[2] = 255;
        if ((((x/32)+(y/32))%2 == 0) && !(gx || gy)) { p[0] /= 2; p[1] /= 2; p[2] /= 2; }
    }
    NSImage *im = [[NSImage alloc] initWithSize:NSMakeSize(W, H)]; [im addRepresentation:rep]; return im;
}

static void Dump(NSMutableString *s, NSString *indent, NSString *title, id obj) {
    // all Objective-C properties of obj (class chain up to NSObject) with their current values
    for (Class c = [obj class]; c && c != [NSObject class]; c = class_getSuperclass(c)) {
        unsigned n = 0; objc_property_t *pl = class_copyPropertyList(c, &n);
        for (unsigned i = 0; i < n; i++) {
            NSString *k = @(property_getName(pl[i])); id v = nil;
            @try { v = [obj valueForKey:k]; } @catch (id e) { v = @"<unreadable>"; }
            NSString *vs = v ? [[[v description] stringByReplacingOccurrencesOfString:@"\n" withString:@" "] copy] : @"nil";
            if (vs.length > 300) vs = [[vs substringToIndex:300] stringByAppendingString:@"..."];
            [s appendFormat:@"%@%@.%@ = %@\n", indent, title, k, vs];
        }
        free(pl);
    }
}

static void DumpEffect(NSMutableString *s, NSString *ind, id e, int depth) {
    if (depth > 6) return;
    NSString *i2 = [ind stringByAppendingString:[@"" stringByPaddingToLength:2 + depth*2 withString:@" " startingAtIndex:0]];
    [s appendFormat:@"%@effect <%@>\n", i2, NSStringFromClass([e class])];
    Dump(s, [i2 stringByAppendingString:@"  "], @"effect", e);
    for (Class c = [e class]; c && c != [NSObject class]; c = class_getSuperclass(c)) {   // nested effects
        unsigned n = 0; objc_property_t *pl = class_copyPropertyList(c, &n);
        for (unsigned k = 0; k < n; k++) { id v = nil; @try { v = [e valueForKey:@(property_getName(pl[k]))]; } @catch (id x) {}
            NSArray *arr = [v isKindOfClass:[NSArray class]] ? v : (v ? @[v] : @[]);
            for (id sub in arr) if ([NSStringFromClass([sub class]) hasPrefix:@"CASDF"] && sub != e) DumpEffect(s, ind, sub, depth + 1); }
        free(pl);
    }
}

static void DumpLayer(NSMutableString *s, CALayer *l, int depth) {
    NSString *ind = [@"" stringByPaddingToLength:depth*2 withString:@" " startingAtIndex:0];
    [s appendFormat:@"%@<%@> frame=%@ bounds=%@ cornerRadius=%g opacity=%g\n", ind, NSStringFromClass([l class]), NSStringFromRect(l.frame), NSStringFromRect(l.bounds), l.cornerRadius, l.opacity];
    NSArray *lists = @[@"filters", @"backgroundFilters"];
    for (NSString *name in lists) {
        NSArray *fs = [l valueForKey:name];
        for (id f in fs) {
            [s appendFormat:@"%@  %@: %@\n", ind, name, f];
            @try {
                NSArray *ks = [f respondsToSelector:NSSelectorFromString(@"inputKeys")] ? [f valueForKey:@"inputKeys"] : nil;
                for (NSString *k in ks) { id v = nil; @try { v = [f valueForKey:k]; } @catch (id e) {}
                    [s appendFormat:@"%@    %@ = %@\n", ind, k, v ? [[v description] stringByReplacingOccurrencesOfString:@"\n" withString:@" "] : @"nil"]; }
            } @catch (id e) {}
        }
    }
    if (l.compositingFilter) [s appendFormat:@"%@  compositingFilter: %@\n", ind, l.compositingFilter];
    for (NSString *key in @[@"effect", @"effects"]) {   // CASDFLayer.effect (and arrays of effects)
        if (![l respondsToSelector:NSSelectorFromString(key)]) continue;
        id v = [l valueForKey:key]; NSArray *es = [v isKindOfClass:[NSArray class]] ? v : (v ? @[v] : @[]);
        for (id e in es) DumpEffect(s, ind, e, 0);
    }
    if (depth == 0 || [NSStringFromClass([l class]) hasPrefix:@"CASDF"] || [NSStringFromClass([l class]) containsString:@"Glass"]) Dump(s, [ind stringByAppendingString:@"  "], @"layer", l);
    for (CALayer *c in l.sublayers) DumpLayer(s, c, depth + 1);
}

@interface ProbeWindow : NSWindow @end
@implementation ProbeWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

static NSString *WindowNumberOf(NSWindow *w) { return [NSString stringWithFormat:@"%ld", (long)w.windowNumber]; }

static NSView *Glass(Class G, NSRect f, CGFloat r, NSInteger style, NSMutableString *rep) {
    NSView *g = [[G alloc] initWithFrame: f];
    @try { [g setValue:@(style) forKey:@"style"]; } @catch (id e) { [rep appendFormat:@"style: %@\n", e]; }
    @try { [g setValue:@(r) forKey:@"cornerRadius"]; } @catch (id e) { [rep appendFormat:@"cornerRadius: %@\n", e]; }
    return g;
}
static void SceneCC(NSString *name, BOOL container, void (^done)(void)) {
    NSWindow *win = [[ProbeWindow alloc] initWithContentRect:NSMakeRect(80, 120, W, H) styleMask:(NSWindowStyleMaskBorderless) backing:NSBackingStoreBuffered defer:NO];
    win.releasedWhenClosed = NO; win.opaque = YES; win.hasShadow = NO;
    NSImageView *bg = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, W, H)]; bg.image = TestBackground(); bg.imageScaling = NSImageScaleNone; win.contentView = bg;
    Class G = NSClassFromString(@"NSGlassEffectView"), C = NSClassFromString(@"NSGlassEffectContainerView");
    NSMutableString *report = [NSMutableString stringWithFormat:@"scene %@ container=%d\nmacOS %@\n", name, container, NSProcessInfo.processInfo.operatingSystemVersionString];
    NSView *top = nil;
    if (G) {
        NSView *holder = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, W, H)];
        struct { CGFloat x, y, w, h, r; } sh[] = { {116, 46, 280, 420, 37}, {126, 390, 130, 63, 31.5}, {126, 318, 130, 63, 31.5}, {264, 318, 122, 135, 24}, {126, 246, 63, 63, 31.5}, {193, 246, 63, 63, 31.5}, {264, 246, 122, 63, 31.5}, {126, 170, 260, 63, 24}, {126, 98, 260, 63, 24} };
        for (int i = 0; i < 9; i++) [holder addSubview: Glass(G, NSMakeRect(sh[i].x, sh[i].y, sh[i].w, sh[i].h), sh[i].r, 0, report)];
        if (container && C) { NSView *cv = [[C alloc] initWithFrame:NSMakeRect(0, 0, W, H)]; @try { [cv setValue: holder forKey:@"contentView"]; [cv setValue:@8 forKey:@"spacing"]; } @catch (id e) { [report appendFormat:@"container: %@\n", e]; } top = cv; }
        else top = holder;
        [bg addSubview: top];
    } else [report appendString:@"NSGlassEffectView not available\n"];
    [win makeKeyAndOrderFront:nil]; [NSApp activateIgnoringOtherApps:YES];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5*NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (top) { [report appendString:@"\n--- layer tree ---\n"]; [top layoutSubtreeIfNeeded]; DumpLayer(report, top.layer, 0); }
        [report writeToFile:[gOut stringByAppendingFormat:@"/%@.txt", name] atomically:YES encoding:NSUTF8StringEncoding error:nil];
        NSTask *t = [NSTask launchedTaskWithLaunchPath:@"/usr/sbin/screencapture" arguments:@[@"-x", @"-o", @"-l", WindowNumberOf(win), [gOut stringByAppendingFormat:@"/%@.png", name]]];
        [t waitUntilExit]; [win orderOut:nil]; done();
    });
}
int main(int argc, char **argv) {
    @autoreleasepool {
        gOut = argc > 1 ? @(argv[1]) : @"/tmp/cc-probe-out";
        [[NSFileManager defaultManager] createDirectoryAtPath:gOut withIntermediateDirectories:YES attributes:nil error:nil];
        NSApplication *app = [NSApplication sharedApplication]; [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSBitmapImageRep *r = (NSBitmapImageRep *)TestBackground().representations.firstObject;
        [[r representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:[gOut stringByAppendingString:@"/background.png"] atomically:YES];
        __block int i = 0; __block void (^next)(void) = nil;
        void (^step)(void) = ^{ if (i >= 2) { printf("done: %s\n", gOut.UTF8String); [NSApp terminate:nil]; return; } int k = i++; SceneCC(k == 0 ? @"cc-container" : @"cc-plain", k == 0, next); };
        next = [step copy]; dispatch_async(dispatch_get_main_queue(), next); [app run];
    }
    return 0;
}
