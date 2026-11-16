#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#include <dlfcn.h>
@interface CAFilter : NSObject
+ (instancetype)filterWithType:(NSString*)t;
- (NSArray*)inputKeys;
@end
int main(void){ @autoreleasepool {
 void *qc=dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore",RTLD_NOW);
 unsigned (*atom)(CFStringRef,int)=dlsym(qc,"CAInternAtom"); printf("CAInternAtom=%p\n",atom); if(!atom) return 1;
 for (NSString *t in @[@"glassBackground",@"glassForeground"]) {
  CAFilter *f=[CAFilter filterWithType:t]; printf("== %s\n",[t UTF8String]);
  for(NSString*k in [f inputKeys]) printf("0x%x %s\n", atom((__bridge CFStringRef)k,0), [k UTF8String]);
 } } return 0; }
