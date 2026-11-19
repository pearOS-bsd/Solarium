// Tone of the regular glass: one glass pill (140 x 63, r 31.5) and one disc (63 x 63) over uniform backgrounds; saves a PNG of each window. PEAR_LIGHT=1: the light appearance.
#import <Cocoa/Cocoa.h>
static NSString *gOut;
@interface PW : NSWindow @end
@implementation PW
- (BOOL)canBecomeKeyWindow { return YES; }
@end
static void Scene(NSString *name, NSColor *col, void (^done)(void)) {
	int W = 300, H = 200; NSWindow *win = [[PW alloc] initWithContentRect:NSMakeRect(80, 120, W, H) styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO]; win.releasedWhenClosed = NO; win.opaque = YES; win.hasShadow = NO;
	NSView *bg = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, W, H)]; bg.wantsLayer = YES; bg.layer.backgroundColor = col.CGColor; win.contentView = bg;
	Class G = NSClassFromString(@"NSGlassEffectView");
	struct { CGFloat x, y, w, h, r; } sh[] = { { 20, 70, 140, 63, 31.5 }, { 200, 70, 63, 63, 31.5 } };
	for (int i = 0; i < 2; i++) { NSView *g = [[G alloc] initWithFrame:NSMakeRect(sh[i].x, sh[i].y, sh[i].w, sh[i].h)]; [g setValue:@0 forKey:@"style"]; [g setValue:@(sh[i].r) forKey:@"cornerRadius"]; [bg addSubview:g]; }
	[win makeKeyAndOrderFront:nil]; [NSApp activateIgnoringOtherApps:YES];
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		NSTask *t = [NSTask launchedTaskWithLaunchPath:@"/usr/sbin/screencapture" arguments:@[@"-x", @"-o", @"-l", [NSString stringWithFormat:@"%ld", (long)win.windowNumber], [gOut stringByAppendingFormat:@"/%@.png", name]]]; [t waitUntilExit]; [win orderOut:nil]; done(); });
}
int main(int argc, char **argv) { @autoreleasepool {
	gOut = argc > 1 ? @(argv[1]) : @"/tmp/pear-tone"; [[NSFileManager defaultManager] createDirectoryAtPath:gOut withIntermediateDirectories:YES attributes:nil error:nil];
	NSApplication *app = [NSApplication sharedApplication]; [app setActivationPolicy:NSApplicationActivationPolicyRegular]; if (getenv("PEAR_LIGHT")) [app setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
	NSMutableArray *sc = [NSMutableArray array];
	for (int g = 0; g <= 255; g += 32) [sc addObject:@[[NSString stringWithFormat:@"grey%03d", g], [NSColor colorWithSRGBRed:g/255.0 green:g/255.0 blue:g/255.0 alpha:1]]]; [sc addObject:@[@"grey255", [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:1]]];
	double cs[][3] = { {0.9,0.2,0.2}, {0.2,0.8,0.2}, {0.2,0.3,0.9}, {0.9,0.8,0.2}, {0.2,0.8,0.8}, {0.8,0.2,0.8}, {0.7,0.85,0.4}, {0.45,0.55,0.0}, {0.75,0.7,0.95} };
	for (int i = 0; i < 9; i++) [sc addObject:@[[NSString stringWithFormat:@"col%d", i], [NSColor colorWithSRGBRed:cs[i][0] green:cs[i][1] blue:cs[i][2] alpha:1]]];
	__block int i = 0; __block void (^next)(void) = nil;
	void (^step)(void) = ^{ if (i >= (int)sc.count) { printf("done\n"); [NSApp terminate:nil]; return; } NSArray *s = sc[i++]; Scene(s[0], s[1], next); };
	next = [step copy]; dispatch_async(dispatch_get_main_queue(), next); [app run]; } return 0; }
