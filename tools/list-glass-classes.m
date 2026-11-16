#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <dlfcn.h>
#import <mach-o/dyld.h>
int main(void){ @autoreleasepool {
 const char *fw[]={"AppKit","QuartzCore","SwiftUICore","SwiftUI","MaterialKit","SkyLight","UIFoundation","CoreUI","Solarium","WindowManagement","LiquidGlass","SolariumKit"};
 for(int i=0;i<12;i++){ char p[256]; snprintf(p,256,"/System/Library/Frameworks/%s.framework/%s",fw[i],fw[i]); void*h=dlopen(p,RTLD_NOW); if(!h){ snprintf(p,256,"/System/Library/PrivateFrameworks/%s.framework/%s",fw[i],fw[i]); h=dlopen(p,RTLD_NOW);} printf("%-18s %s\n",fw[i],h?"loaded":"-"); }
 unsigned n=0; Class *cs=objc_copyClassList(&n); printf("classes: %u\n",n);
 for(unsigned i=0;i<n;i++){ const char*nm=class_getName(cs[i]); if(strcasestr(nm,"solarium")||strcasestr(nm,"glass")||strcasestr(nm,"liquid")){ const char *img=class_getImageName(cs[i]); printf("%-48s %s\n",nm,img?strrchr(img,'/')+1:"?"); } }
 uint32_t ni=_dyld_image_count(); for(uint32_t i=0;i<ni;i++){ const char*p=_dyld_get_image_name(i); if(strcasestr(p,"solarium")||strcasestr(p,"glass")) printf("IMAGE %s\n",p); }
}return 0;}
