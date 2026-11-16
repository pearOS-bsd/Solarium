/* BSD-2-Clause. Runs a glass_foreground_sdf vertex+fragment SPIR-V pair (made by metal2vulkan from
 * Apple's AIR) offscreen on the Vulkan device and writes the result as raw RGBA8 (out.rgba) plus the
 * background used (bg.rgba).  Parameters: the Apple defaults of docs/APPLE-DECISIONS.md.
 * usage: run VERT.spv FRAG.spv DEVICE_INDEX KIND(0 rect, 4 squircle) */
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define W 512
#define H 512
#define CK(x) do{VkResult r_=(x); if(r_){fprintf(stderr,"%s failed: %d (line %d)\n",#x,r_,__LINE__); exit(1);}}while(0)
static VkPhysicalDevice pd; static VkDevice dev;
static uint32_t memtype(uint32_t bits, VkMemoryPropertyFlags f){VkPhysicalDeviceMemoryProperties m;vkGetPhysicalDeviceMemoryProperties(pd,&m);
 for(uint32_t i=0;i<m.memoryTypeCount;i++) if((bits&(1u<<i))&&(m.memoryTypes[i].propertyFlags&f)==f) return i; fprintf(stderr,"no memtype\n");exit(1);}
static void mkbuf(VkDeviceSize sz,VkBufferUsageFlags u,VkBuffer*b,VkDeviceMemory*m,void**map){
 VkBufferCreateInfo bi={VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,0,0,sz,u,VK_SHARING_MODE_EXCLUSIVE};CK(vkCreateBuffer(dev,&bi,0,b));
 VkMemoryRequirements r;vkGetBufferMemoryRequirements(dev,*b,&r);
 VkMemoryAllocateInfo ai={VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,0,r.size,memtype(r.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
 CK(vkAllocateMemory(dev,&ai,0,m));CK(vkBindBufferMemory(dev,*b,*m,0));CK(vkMapMemory(dev,*m,0,sz,0,map));}
static uint32_t gMips=1;
static void mkimg(VkFormat f,VkImageUsageFlags u,VkImage*im,VkDeviceMemory*m,VkImageView*v){
 VkImageCreateInfo ii={VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ii.imageType=VK_IMAGE_TYPE_2D;ii.format=f;ii.extent=(VkExtent3D){W,H,1};ii.mipLevels=(u&VK_IMAGE_USAGE_SAMPLED_BIT)?gMips:1;ii.arrayLayers=1;ii.samples=VK_SAMPLE_COUNT_1_BIT;ii.tiling=VK_IMAGE_TILING_OPTIMAL;ii.usage=u;ii.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
 CK(vkCreateImage(dev,&ii,0,im));VkMemoryRequirements r;vkGetImageMemoryRequirements(dev,*im,&r);
 VkMemoryAllocateInfo ai={VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,0,r.size,memtype(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
 CK(vkAllocateMemory(dev,&ai,0,m));CK(vkBindImageMemory(dev,*im,*m,0));
 VkImageViewCreateInfo vi={VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=*im;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=f;vi.subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT,0,(u&VK_IMAGE_USAGE_SAMPLED_BIT)?gMips:1,0,1};CK(vkCreateImageView(dev,&vi,0,v));}
static VkShaderModule mod(const char*p){FILE*f=fopen(p,"rb");if(!f){perror(p);exit(1);}fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint32_t*d=malloc(n);fread(d,1,n,f);fclose(f);
 VkShaderModuleCreateInfo si={VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,0,0,n,d};VkShaderModule m;CK(vkCreateShaderModule(dev,&si,0,&m));return m;}
static void barrier(VkCommandBuffer c,VkImage im,VkImageLayout o,VkImageLayout n,VkAccessFlags sa,VkAccessFlags da,VkPipelineStageFlags ss,VkPipelineStageFlags ds){
 VkImageMemoryBarrier b={VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,0,sa,da,o,n,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,im,{VK_IMAGE_ASPECT_COLOR_BIT,0,VK_REMAINING_MIP_LEVELS,0,1}};
 vkCmdPipelineBarrier(c,ss,ds,0,0,0,0,0,1,&b);}
int main(int argc,char**argv){
 if(argc<5){fprintf(stderr,"usage: run VERT FRAG DEV KIND\n");return 2;}
 int kind=atoi(argv[4]);
 const char*blockfile=argc>5?argv[5]:0; if(blockfile) gMips=10;
 VkApplicationInfo ai={VK_STRUCTURE_TYPE_APPLICATION_INFO};ai.apiVersion=VK_API_VERSION_1_3;
 VkInstanceCreateInfo ic={VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,0,0,&ai};VkInstance inst;CK(vkCreateInstance(&ic,0,&inst));
 uint32_t n=0;vkEnumeratePhysicalDevices(inst,&n,0);VkPhysicalDevice pds[8];if(n>8)n=8;vkEnumeratePhysicalDevices(inst,&n,pds);
 int di=atoi(argv[3]);pd=pds[di];VkPhysicalDeviceProperties pp;vkGetPhysicalDeviceProperties(pd,&pp);printf("device: %s\n",pp.deviceName);
 float prio=1;VkDeviceQueueCreateInfo qi={VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,0,0,0,1,&prio};
 uint32_t qn=0;vkGetPhysicalDeviceQueueFamilyProperties(pd,&qn,0);VkQueueFamilyProperties qp[16];if(qn>16)qn=16;vkGetPhysicalDeviceQueueFamilyProperties(pd,&qn,qp);
 for(uint32_t i=0;i<qn;i++) if(qp[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){qi.queueFamilyIndex=i;break;}
 VkPhysicalDeviceVulkan13Features f13={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};f13.dynamicRendering=1;
 VkPhysicalDeviceVulkan12Features f12={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,&f13};f12.shaderFloat16=1;f12.scalarBlockLayout=1;f12.uniformBufferStandardLayout=1;
 VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR fmr={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MAXIMAL_RECONVERGENCE_FEATURES_KHR,&f12};fmr.shaderMaximalReconvergence=1;
 const char*ext[]={"VK_KHR_shader_maximal_reconvergence"};
 VkDeviceCreateInfo dc={VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,&fmr,0,1,&qi,0,0,1,ext};CK(vkCreateDevice(pd,&dc,0,&dev));
 VkQueue q;vkGetDeviceQueue(dev,qi.queueFamilyIndex,0,&q);
 /* images */
 VkImage cimg,timg;VkDeviceMemory cm,tm;VkImageView cv,tv;
 mkimg(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,&cimg,&cm,&cv);
 mkimg(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,&timg,&tm,&tv);
 VkBuffer sb;VkDeviceMemory sm;unsigned char*sp;mkbuf(W*H*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,&sb,&sm,(void**)&sp);
 for(int y=0;y<H;y++)for(int x=0;x<W;x++){unsigned char*p=sp+(y*W+x)*4;int gx=(x%64<3),gy=(y%64<3);
  p[0]=(unsigned char)(255*x/W);p[1]=(unsigned char)(255*y/H);p[2]=(unsigned char)(255-255*x/W);p[3]=255;
  if(gx||gy){p[0]=p[1]=p[2]=255;} if(((x/32)+(y/32))%2==0&&!(gx||gy)){p[0]/=2;p[1]/=2;p[2]/=2;}}
 FILE*fb=fopen("bg.rgba","wb");fwrite(sp,1,W*H*4,fb);fclose(fb);
 VkSampler smp;VkSamplerCreateInfo sci={VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sci.magFilter=sci.minFilter=VK_FILTER_LINEAR;sci.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;sci.maxLod=16;sci.addressModeU=sci.addressModeV=sci.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;CK(vkCreateSampler(dev,&sci,0,&smp));
 /* uniform buffers */
 VkBuffer u0,u1,u2;VkDeviceMemory um0,um1,um2;float*p0,*p1,*p2;
 mkbuf(128,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&u0,&um0,(void**)&p0);mkbuf(384,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&u1,&um1,(void**)&p1);mkbuf(80,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&u2,&um2,(void**)&p2);
 memset(p0,0,128);memset(p1,0,384);memset(p2,0,80);
 p0[0]=p0[5]=p0[10]=p0[15]=1; /* identity clip matrix */
 /* block 0: half extents, kind, mix */ p1[0]=170;p1[1]=100;p1[2]=(float)kind;p1[3]=0;
 /* block 1: 2x2 mapping of the normal */ p1[4]=1;p1[5]=0;p1[6]=0;p1[7]=1;
 /* block 2: curvature flags, corner radius */ p1[8]=0;p1[9]=0;p1[10]=60;p1[11]=0;
 /* block 3: layer matrix in uv units */ p1[12]=1.0f/W;p1[13]=0;p1[14]=0;p1[15]=1.0f/H;
 p1[16]=-150;p1[17]=1.0f/100;p1[18]=-10;      /* refraction amount, 1/height, offset */
 p1[19]=-15;p1[20]=1.0f/20;p1[21]=0;p1[22]=0;p1[23]=1;   /* aberration amount, 1/height, offset, sin, cos */
 p1[24]=-4.5f;p1[25]=-3.0f;p1[26]=0;p1[27]=1;  /* edge start, end, opacity start, end */
 p1[28]=0;p1[29]=1;                           /* refraction sin, cos */
 p2[0]=1; /* intensity */
 if(blockfile){FILE*bf=fopen(blockfile,"rb");if(!bf){perror(blockfile);return 1;}size_t rn=fread(p1,1,384,bf);fclose(bf);printf("block from %s (%zu bytes)\n",blockfile,rn);}
 /* pipeline */
 VkDescriptorSetLayoutBinding b[4]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT},{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT},{2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT},{3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT}};
 VkDescriptorSetLayoutCreateInfo dl={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,0,0,4,b};VkDescriptorSetLayout dsl;CK(vkCreateDescriptorSetLayout(dev,&dl,0,&dsl));
 VkPushConstantRange pr[2]={{VK_SHADER_STAGE_VERTEX_BIT,0,4},{VK_SHADER_STAGE_FRAGMENT_BIT,64,8}};
 VkPipelineLayoutCreateInfo pl={VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,0,0,1,&dsl,2,pr};VkPipelineLayout lay;CK(vkCreatePipelineLayout(dev,&pl,0,&lay));
 VkShaderModule vm=mod(argv[1]),fm=mod(argv[2]);
 VkBool32 vspec[1]={1};VkSpecializationMapEntry vme[1]={{1,0,4}};VkSpecializationInfo vsi={1,vme,4,vspec};
 uint32_t fspec[3]={0,1,1};VkSpecializationMapEntry fme[3]={{0,0,4},{2,4,4},{3,8,4}};VkSpecializationInfo fsi={3,fme,12,fspec};
 VkPipelineShaderStageCreateInfo st[2]={{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,0,0,VK_SHADER_STAGE_VERTEX_BIT,vm,"main",&vsi},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,0,0,VK_SHADER_STAGE_FRAGMENT_BIT,fm,"main",&fsi}};
 VkVertexInputBindingDescription vb={0,32,VK_VERTEX_INPUT_RATE_VERTEX};
 VkVertexInputAttributeDescription va[3]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32_SFLOAT,16},{2,0,VK_FORMAT_R32G32_SFLOAT,24}};
 VkPipelineVertexInputStateCreateInfo vi={VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,0,0,1,&vb,3,va};
 VkPipelineInputAssemblyStateCreateInfo ia={VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,0,0,VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
 VkViewport vp={0,0,W,H,0,1};VkRect2D sc={{0,0},{W,H}};VkPipelineViewportStateCreateInfo vs={VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,0,0,1,&vp,1,&sc};
 VkPipelineRasterizationStateCreateInfo rs={VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;
 VkPipelineMultisampleStateCreateInfo ms={VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
 VkPipelineColorBlendAttachmentState ba={0};ba.colorWriteMask=0xF;VkPipelineColorBlendStateCreateInfo cb={VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,0,0,0,0,1,&ba};
 VkFormat cf=VK_FORMAT_R8G8B8A8_UNORM;VkPipelineRenderingCreateInfo ri={VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,0,0,1,&cf};
 VkGraphicsPipelineCreateInfo gp={VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,&ri,0,2,st,&vi,&ia,0,&vs,&rs,&ms,0,&cb,0,lay};VkPipeline pipe;CK(vkCreateGraphicsPipelines(dev,0,1,&gp,0,&pipe));
 /* quad: clip pos, local (pixels from centre), uv */
 float vx[6*8];float cx[6]={-1,1,1,-1,1,-1},cy[6]={-1,-1,1,-1,1,1};
 for(int i=0;i<6;i++){float*v=vx+i*8;v[0]=cx[i];v[1]=cy[i];v[2]=0;v[3]=1;float u=(cx[i]+1)/2,w=(cy[i]+1)/2;v[4]=(u-0.5f)*W;v[5]=(w-0.5f)*H;v[6]=u;v[7]=w;}
 VkBuffer vbuf;VkDeviceMemory vmem;void*vmap;mkbuf(sizeof vx,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,&vbuf,&vmem,&vmap);memcpy(vmap,vx,sizeof vx);
 VkDescriptorPoolSize ps[2]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,3},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1}};VkDescriptorPoolCreateInfo dpc={VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,0,0,1,2,ps};VkDescriptorPool dp;CK(vkCreateDescriptorPool(dev,&dpc,0,&dp));
 VkDescriptorSetAllocateInfo da={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,0,dp,1,&dsl};VkDescriptorSet ds;CK(vkAllocateDescriptorSets(dev,&da,&ds));
 VkDescriptorBufferInfo bi0={u0,0,128},bi1={u1,0,384},bi2={u2,0,80};VkDescriptorImageInfo ii={smp,tv,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
 VkWriteDescriptorSet wr[4]={{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,0,ds,0,0,1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&bi0},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,0,ds,1,0,1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&bi1},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,0,ds,2,0,1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&bi2},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,0,ds,3,0,1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,&ii}};
 vkUpdateDescriptorSets(dev,4,wr,0,0);
 VkCommandPoolCreateInfo cpi={VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,0,0,qi.queueFamilyIndex};VkCommandPool cp;CK(vkCreateCommandPool(dev,&cpi,0,&cp));
 VkCommandBufferAllocateInfo cai={VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,0,cp,VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};VkCommandBuffer cmd;CK(vkAllocateCommandBuffers(dev,&cai,&cmd));
 VkCommandBufferBeginInfo cbi={VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};CK(vkBeginCommandBuffer(cmd,&cbi));
 barrier(cmd,timg,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
 VkBufferImageCopy rg={0,0,0,{VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},{0,0,0},{W,H,1}};vkCmdCopyBufferToImage(cmd,sb,timg,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&rg);
 for(uint32_t m=1;m<gMips;m++){ /* mip chain: level m-1 -> m, box filter by linear blit */
  VkImageMemoryBarrier pb={VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,timg,{VK_IMAGE_ASPECT_COLOR_BIT,m-1,1,0,1}};
  vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,0,0,0,1,&pb);
  int sw=W>>(m-1),sh=H>>(m-1),dw=sw>1?sw/2:1,dh=sh>1?sh/2:1;
  VkImageBlit bl={{VK_IMAGE_ASPECT_COLOR_BIT,m-1,0,1},{{0,0,0},{sw,sh,1}},{VK_IMAGE_ASPECT_COLOR_BIT,m,0,1},{{0,0,0},{dw,dh,1}}};
  vkCmdBlitImage(cmd,timg,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,timg,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&bl,VK_FILTER_LINEAR);}
 if(gMips>1){ VkImageMemoryBarrier lb={VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,timg,{VK_IMAGE_ASPECT_COLOR_BIT,gMips-1,1,0,1}};
  vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,0,0,0,1,&lb);
  barrier(cmd,timg,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);}
 else
 barrier(cmd,timg,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
 barrier(cmd,cimg,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
 VkRenderingAttachmentInfo ra={VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};ra.imageView=cv;ra.imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;ra.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;ra.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
 VkRenderingInfo rin={VK_STRUCTURE_TYPE_RENDERING_INFO};rin.renderArea=sc;rin.layerCount=1;rin.colorAttachmentCount=1;rin.pColorAttachments=&ra;
 vkCmdBeginRendering(cmd,&rin);vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipe);vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,lay,0,1,&ds,0,0);
 uint32_t z[2]={0,0};vkCmdPushConstants(cmd,lay,VK_SHADER_STAGE_VERTEX_BIT,0,4,z);vkCmdPushConstants(cmd,lay,VK_SHADER_STAGE_FRAGMENT_BIT,64,8,z);
 VkDeviceSize off=0;vkCmdBindVertexBuffers(cmd,0,1,&vbuf,&off);vkCmdDraw(cmd,6,1,0,0);vkCmdEndRendering(cmd);
 barrier(cmd,cimg,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
 vkCmdCopyImageToBuffer(cmd,cimg,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,sb,1,&rg);CK(vkEndCommandBuffer(cmd));
 VkSubmitInfo si={VK_STRUCTURE_TYPE_SUBMIT_INFO,0,0,0,0,1,&cmd};CK(vkQueueSubmit(q,1,&si,0));CK(vkQueueWaitIdle(q));
 FILE*fo=fopen("out.rgba","wb");fwrite(sp,1,W*H*4,fo);fclose(fo);printf("wrote out.rgba and bg.rgba (%dx%d)\n",W,H);return 0;}
