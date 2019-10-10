#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dec_func( const char* src, int src_len, char* dest, int *dest_len )
{
   int ret = 0;
   int i = src_len-1;
   int j = 0;
   
   *dest_len = src_len;
   
   for( ; i>-1; i--,j++ )
   {
       *(dest+j) = *(src+i);
   }
   
   printf("  原串=[%*.*s], 解密长度=[%d]\n", src_len, src_len, src, src_len); 
   printf("  加密=[%*.*s], 解密长度=[%d]\n", *dest_len, *dest_len, dest, src_len); 
      
   return ret; 
}