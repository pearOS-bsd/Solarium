#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
@interface CAFilter : NSObject
+ (instancetype)filterWithType:(NSString*)t;
- (NSArray*)inputKeys;
- (id)valueForKey:(NSString*)k;
@end
int main(void){ @autoreleasepool {
 for (NSString *t in @[@"glassBackground",@"glassForeground"]) {
  CAFilter *f=[CAFilter filterWithType:t]; if(!f){printf("%s: nil\n",[t UTF8String]);continue;}
  NSArray *ks=[f inputKeys]; printf("== %s: %lu keys\n",[t UTF8String],(unsigned long)ks.count);
  for(NSString*k in ks){ id v=nil; @try{ v=[f valueForKey:k]; }@catch(id e){} printf("  %-34s %s\n",[k UTF8String],[[[v description] stringByReplacingOccurrencesOfString:@"\n" withString:@" "] UTF8String]); }
 } } return 0; }
