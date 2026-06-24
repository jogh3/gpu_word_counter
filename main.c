#include <stdio.h>
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdlib.h>

typedef struct {
  char* data;
  long arr_size;
} data_struct;

void split_check(long *start, data_struct *data){
   if (data == NULL || data->arr_size <= 0) {
       return; 
   }
   for (int i = data->arr_size-1; i >= 0; i--){
    if (data->data[i] == ' '){
      int size_difference = data->arr_size - i;
      if (start != NULL) {
          *start -= size_difference+1; 
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

int main(int argc,char* argv[]){
  cl_int err;
  cl_platform_id platform;
  cl_device_id device;
  cl_ulong vram_size, available_vram;
  err = clGetPlatformIDs(1, &platform, NULL);
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
  FILE *fptr;
  char* filename = argv[1];
  fptr = fopen(filename, "r");
  if(fptr == NULL) {printf("unable to open file"); return 1;}
  int counter;
  clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE,sizeof(vram_size), &vram_size,NULL);
  available_vram = vram_size/2;
  fseek(fptr, 0, SEEK_END);
  long f_size = ftell(fptr);
  rewind(fptr);
  long start_pos = 0;
  while (start_pos < f_size){
    data_struct data = get_file_data(fptr, available_vram, &start_pos ,f_size);
    printf("%.*s", data.arr_size, data.data);
    // gpu exection
    free(data.data);
  }
  fclose(fptr);
  return 0;
}
