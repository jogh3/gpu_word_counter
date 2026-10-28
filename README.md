# gpu word counter

A High performance word counter that offloads the counter onto the GPU, using OpenCL, separating the file into chunks ensuring it doesn't fill up vram with the data bricking the computer

# Limitations

it only works on pure ASCII text, anything bigger and it exits with a failure exit code

i also lazily accounted for what counts as a separator between a new word, so multiple punctuation in a row can screw with the count if compared to wc

it works best on big files, i tested with a 1GiB txt file in which it was faster than wc -w, especially on an N150 with integrated graphics

# Prerequisites and compilation

 - ensure a C compiler is installed(e.g gcc or clang)
 - install OpenCL headers
 - an OpenCL ICD loader
 - a GPU driver (e.g rocm-opencl-runtime for amd,. intel-compute-runtime for intel, or nvidia drivers)

```
gcc -o main main.c -lOpenCL
```
# Usage

```
./main "your text file.txt
```
