#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
int main(void){ @autoreleasepool {
 const char *names[]={"CASDFGlassDisplacementEffect","CASDFGlassHighlightEffect","CASDFShadowEffect","CASDFFillEffect","CASDFGradientEffect","CASDFKeyFillHighlightEffect","CASDFGradientContourEffect","CASDFOutputEffect"};
 for(int i=0;i<8;i++){ Class c=NSClassFromString(@(names[i])); if(!c){ printf("%s: no class\n",names[i]); continue; }
  id name=[c respondsToSelector:@selector(name)]?[c performSelector:@selector(name)]:nil;
  NSDictionary *d=[c respondsToSelector:NSSelectorFromString(@"defaultValues")]?[c performSelector:NSSelectorFromString(@"defaultValues")]:nil;
  printf("== %s  name=%s\n",names[i],[[name description] UTF8String]); NSArray *keys=[[d allKeys] sortedArrayUsingSelector:@selector(compare:)]; for(NSString*k in keys) printf("   %-14s = %s\n",[k UTF8String],[[d[k] description] UTF8String]);
  id attrs=[c respondsToSelector:NSSelectorFromString(@"CA_attributes")]?[c performSelector:NSSelectorFromString(@"CA_attributes")]:nil; if(attrs) printf("   CA_attributes: %s\n",[[[attrs description] stringByReplacingOccurrencesOfString:@"\n" withString:@" "] UTF8String]);
 } }return 0;}
