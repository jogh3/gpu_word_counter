#include <stddef.h>
#include <stdio.h>
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdlib.h>

#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
    #define ASCII_MASK 0x8080808080808080ULL
#elif SIZE_MAX == 0xFFFFFFFF
    #define ASCII_MASK 0x80808080UL
#else
    #define ASCII_MASK ((uint8_t)0x80)
#endif

int if_ascii(FILE *fptr) {
  size_t buffer[512];
  size_t items_read;
  while ((items_read = fread(buffer, sizeof(ASCII_MASK), 512, fptr)) > 0) {
    for (size_t i = 0; i < items_read; i++){
      if (buffer[i] & ASCII_MASK) {
        fprintf(stderr, "FATAL: file contains non-ascii characters\n");
        return 1;
      }
    }
  }
  fseek(fptr,0,SEEK_SET);
  return 0;
}

typedef struct {
  char* data;
  unsigned long arr_size;
} data_struct;

void split_check(long *start, data_struct *data){
   if (data == NULL || data->arr_size <= 0) {
       return; 
   }
   for (int i = data->arr_size-1; i >= 0; i--){
    if (data->data[i] == ' '){
      int size_difference = data->arr_size - i;
      if (start != NULL) {
          *start -= size_difference-1; 
      }
      data->arr_size = i+1;
      char *temp = realloc(data->data, data->arr_size);
      if (temp != NULL) {
          data->data = temp;
      }
      break;
    }
  }
  return;
}

data_struct get_file_data(FILE *fptr, cl_ulong vram_size, long *start, long f_size){
  data_struct data;
  fseek(fptr, *start, SEEK_SET);
  long remaining_file = f_size - *start;
  if (remaining_file >= vram_size) {
    *start += vram_size; 
    data.arr_size = vram_size;
  } else {
    data.arr_size = remaining_file;
    *start += remaining_file;
  }
  data.data = malloc(data.arr_size);
  fread(data.data, sizeof(char), data.arr_size,fptr);
  if (*start != f_size){
    split_check(start, &data);
  }
  return data;
}

long get_full_file_size(FILE *fptr){
  fseek(fptr, 0, SEEK_END);
  long f_size = ftell(fptr);
  rewind(fptr);
  return f_size;
}

int main(int argc,char* argv[]){
  cl_int err;
  cl_platform_id platform;
  cl_device_id device;
  cl_ulong vram_size, available_vram;

  err = clGetPlatformIDs(1, &platform, NULL);
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

  FILE *fptr;
  char* filename = argv[1];
  fptr = fopen(filename, "rb");
  if(fptr == NULL) {printf("unable to open file"); return 1;}

  // this checks if the file is ascii or not, if utf8 or bigger exit
  if (if_ascii(fptr)) return 1;
  FILE *cl_ptr = fopen("./index.cl", "rb");
  long clf_size = get_full_file_size(cl_ptr);
  long cl_start = 0;
  data_struct raw_cl = get_file_data(cl_ptr,clf_size,&cl_start,clf_size);
  fclose(cl_ptr);

  clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,sizeof(cl_ulong), &vram_size,NULL);
  available_vram = vram_size/2;

  long f_size = get_full_file_size(fptr);
  long start_pos = 0;

  cl_context context = clCreateContext(NULL,1,&device,NULL,NULL,&err);
  cl_program program_cl = clCreateProgramWithSource(context,1,(const char **)&raw_cl.data, &raw_cl.arr_size,NULL);
  cl_int prog_build = clBuildProgram(program_cl,1,&device,NULL,NULL,NULL);
  cl_kernel kernel_cl = clCreateKernel(program_cl,"count_words",NULL);
  
  cl_command_queue queue = clCreateCommandQueueWithProperties(context,device,NULL,&err);

  cl_mem text_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, available_vram,NULL,NULL);
  cl_mem counter_buf = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(unsigned int),NULL,NULL);

  unsigned int total_words = 0;
  clEnqueueWriteBuffer(queue,counter_buf,CL_TRUE,0,sizeof(unsigned int), &total_words,0,NULL,NULL);
  while (start_pos < f_size){
    data_struct data = get_file_data(fptr, available_vram, &start_pos ,f_size);
    // printf("%.*s", (int)data.arr_size, data.data);
    clEnqueueWriteBuffer(queue,text_buf,CL_TRUE,0,data.arr_size,data.data,0,NULL,NULL);
    clSetKernelArg(kernel_cl, 0, sizeof(cl_mem), &text_buf);
    clSetKernelArg(kernel_cl,1,sizeof(cl_mem), &counter_buf);
    clSetKernelArg(kernel_cl,2,sizeof(int),&data.arr_size);
    clEnqueueNDRangeKernel(queue,kernel_cl,1,NULL,&data.arr_size,NULL,0,NULL,NULL);
    // gpu exection
    free(data.data);
  }
  clEnqueueReadBuffer(queue,counter_buf,CL_TRUE,0,sizeof(unsigned int),&total_words,0,NULL,NULL);
  fclose(fptr);
  printf("%u\n", total_words);
  return 0;
}

// Inside your while loop:
// clEnqueueWriteBuffer(
//     queue, 
//     text_buf, 
//     CL_TRUE, // Blocking write
//     0,       // Offset 
//     data.arr_size, // ONLY write the actual chunk size!
//     data.data, 
//     0, NULL, NULL
// );
