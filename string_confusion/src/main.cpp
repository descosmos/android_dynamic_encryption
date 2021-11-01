#include <stdio.h>
#include <dlfcn.h>
//#include <stdlib.h>

typedef int(* FUNC_TYPE)(int, int);
int (*add)(int, int) = nullptr;
int (*sub)(int, int) = nullptr;
int (*mul)(int, int) = nullptr;
int (*div)(int, int) = nullptr;

int main(int argc, char *argv[]) {
   char* so_name = argv[1];
   
   printf("before dlopen.\n");
   void *handler = dlopen(so_name, RTLD_NOW | RTLD_GLOBAL);
   if (handler == nullptr) {
      printf("Cannot dlopen %s.\n", so_name);
      return -1;
   }
   printf("dlopen success.\n");

   // add = (FUNC_TYPE)dlsym(handler, "add");
   // sub = (FUNC_TYPE)dlsym(handler, "sub");
   // mul = (FUNC_TYPE)dlsym(handler, "mul");
   // div = (FUNC_TYPE)dlsym(handler, "div");

   // if (!add || !sub || !mul || !div) {
   //    printf("dlsym failed.\n");
   //    return -1;
   // }

   int a = 5, b = 2;
   // printf("add(%d, %d) = %d\n", a, b, add(a,b));
   // printf("sub(%d, %d) = %d\n", a, b, sub(a,b));
   // printf("mul(%d, %d) = %d\n", a, b, mul(a,b));
   // printf("div(%d, %d) = %d\n", a, b, div(a,b));

   dlclose(handler);

   return 0;
}