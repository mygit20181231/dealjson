#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>  
#include <dlfcn.h>



/* 可配置内容最大长度 */
#define MAX_BUF_LEN 4096
/* VALUE最长长度 */
#define MAX_VALUE_LEN  51200
//函数指针
typedef int (*DEC_FUNC)( const char*, int, char*, int * );

/*需要处理的数据*/
char dec_ele[MAX_BUF_LEN+1];
void *handle = NULL;
DEC_FUNC p_dec_func = NULL; 

/*
 * config_file 配置文件，需要全路径
 * 返回值：0 成功
           -1 失败
*/
int load_config( char * config_file )
{
    FILE *fp = NULL;
    char buf[100+1];

    int len = 0;
    int total = 0;
    
    /* 处理数据开始 */
    int dec_flag = 0;
    
    memset( dec_ele, 0x00, sizeof(dec_ele) );
    memset( buf, 0x00, sizeof(buf) );

    if( config_file == NULL )
    {
        printf("config file is wrong.\n");
        return -1;
    }
    
    if( (fp=fopen(config_file,"r")) == NULL )
    {
        printf("open config file <%s> fail.\n", config_file );
        return -1;
    }

    while ( fgets(buf, sizeof(buf), fp) != NULL )
    {
        /* 忽略#,空格,TAB等开头的行 */
        if( (buf[0] == '#') || 
        	  (buf[0] == ' ') || 
        	  (buf[0] == '	') || 
        	  ((len=strlen(buf))<=0) ||
        	  (buf[0] == '\n')
          )
        {
            continue;
        }
        /* [1]开始是需要处理的数据 */
        else if ( !strncmp("[1]",buf,3)  )
        {
            dec_flag = 1;
            dec_ele[0] = '\n';
            continue;
        }
        /* [2]开始是需要加密的数据 */
        else if ( !strncmp("[2]",buf,3) )
        {
            dec_flag = 2;
            /*暂不开发*/
            continue;
        }
        else
        {        
            if( dec_flag==1 )
            {
                total = total + len + 2;
                if( total > MAX_BUF_LEN )
                {
                    printf( "oversize!\n" );
                    return -1;
                }       
                /* 回车做为分隔符 */
                strcat( dec_ele, buf );
                continue;
            }
            printf( "uncorrect line: [%s]\n", buf );
            continue;
        }
    }

    fclose(fp);

    return 0;
}

int dec_data( const char *in_buf, int in_buf_len, char *out_buf, int out_buf_len )
{
   /* KEY第一个双引号位置 */
   const char *p1 = NULL;
   /* KEY第二个双引号位置 */
   const char *p2 = NULL;
   /* VALUE第一个位置 */
   const char *p3 = NULL;
   /* VALUE数组开始位置 */
   const char *p5 = NULL;
  
   /* 当前位置 */
   const char *p_curr = NULL; 
   /* 处理当前位置 */
   char *p_dec_curr = NULL;    
   /* 是否需要处理 */
   char *p_dec = NULL;      
        
   /* KEY名称 */
   char key[200+1];
   /* VALUE值 */
   char value[MAX_VALUE_LEN+1];   
   char tmp[512+1];
   
   /* buf长度 */
   int len = 0;
   /* VALUE长度 */
   int out_len = 0;

    
   /* 解析状态 
      1 解析KEY 
      2 解析语法
      3 解析值
    */
   int mode = 0;
   /* 进行KV解析标志 */
   int kv_flag = 0;
   /* VALUE里数组模式标志 */
   int array_flag = -1;
   /* 0 String, 大于0其它  */   
   int data_type = -1;
   /* 是否在解析VALUE */
   int in_value_flag = -1;
   /* VALUE是哪种类型 */
   int obj_type = -1;   
   int ch_num = 0;
   int i = 0;
   /* 是否在拷贝 */
   int cp_flag = 1;
   /* 处理后数据长度 */
   int dec_off_set = 0;
    
   if( (in_buf == NULL) || (out_buf == NULL) )
   {
      printf( "wrong para\n" );
      return -1;
   }
   len = strlen(in_buf);
         
   p_curr = in_buf;
   p_dec_curr = out_buf;
   mode = 1;
   for( i=0; i<len; i++, p_curr++ )
   {  
   	   if( p_curr == NULL )
   	   {
   	       break;
   	   }
   	   
   	   if( cp_flag == 1 )
   	   {
   	       if( out_len + 1 > out_buf_len )
   	       {
   	           printf("out buf oversize\n");
   	           return -1;
   	       }
   	       *p_dec_curr = *p_curr;
   	       p_dec_curr ++ ;
   	       out_len ++;
   	   }	
   	   
   	   /* 解析KEY部分 */
       if( mode == 1 )
       {	
           if( *p_curr == '"' )
           {
               if( p1 == NULL )
               {
               		p1 = p_curr;
               		continue;
               }
               p2 = p_curr;
               mode = 2;
               kv_flag = 1;
               ch_num = 0; 
           }
           continue;
       }
       /* 解析语法部分 */
       else if ( mode == 2 )
       {
           /* 语法部分忽略空格和TAB */
           if( (*p_curr == ' ') || 
           	   (*p_curr == '	') ||
               (*p_curr == '\r') ||
               (*p_curr == '\n')
           	 )
           {
   	   	       continue;
           }
           ch_num ++;
           
           if( (*p_curr == '}') ||
           	   (*p_curr == ']') 
           	)
           {
               continue;
           }
           else if( 
           	   (*p_curr == '{') || 
           	   (*p_curr == '[') ||
           	   (*p_curr == ',')            	
           	)
           {
               mode = 1;
               continue;
           }	
           else if( *p_curr == ':' )
           {
               /* 证明第二个引号和冒号有其它字符,不是KEY */
               if( ch_num > 1 )
               {
                   continue;
               }
               
               if( kv_flag == 1 )
               {
                   if( *p2 != '"' )
                   {
                       printf( "p2 position error!\n" );
                       return -1;
                   }
                   memset( key, 0x00, sizeof(key) );
                   memcpy( key, p1+1, (p2-p1-1) );
                   p1 = NULL;
                   p2 = NULL;
                   kv_flag = -1;
                   mode = 3; 
                   printf("get key[%s]\n", key );
                   if( strlen(key)>0 )
                   {
                       memset(tmp,0x00,sizeof(tmp));
                       sprintf( tmp, "\n%s\n", key );
                       p_dec = strstr( dec_ele, tmp ); 
                   }      
               }
               continue;		
           }else
           {
           	   continue;  	   
           }
       }
       /* 解析VALUE */
       else if ( mode == 3 )
       {
           /* 未在值内 */
           if( in_value_flag < 0 )
           {
               /* 语法部分忽略空格和TAB */
               if( (*p_curr == ' ') || 
               	   (*p_curr == '	') ||
               	   (*p_curr == '\r') ||
               	   (*p_curr == '\n')
               	  )
               {
   	   	           continue;
               }
               /* 分隔符 */               
               else if ( *p_curr == ',' )  
               {
                   continue;
               }                 
               /* 数组 */
               else if( (*p_curr == '[') ||  (*p_curr == ']') || (obj_type==0) )
               {
                   obj_type = 0;
               }
               /* 字符串 */               
               else if ( *p_curr == '\"' )
               {
                   obj_type = 1;
                   in_value_flag = 1;
               	   if( p_dec != NULL )
               	   {
               	       cp_flag = -1;
               	   }                   
               }
               /* 对象 */               
               else if ( *p_curr == '{' )
               {
                   p3 = NULL;
                   p5 = NULL;
                   obj_type = -1;
                   data_type = -1;
                   in_value_flag = -1;
           	           
                   mode = 1;   
               }                           
               /* 其它数据类型 */
               else
               {    
               	   obj_type = 2;
               	   in_value_flag = 1;
               	   if( p_dec != NULL )
               	   {
               	       cp_flag = -1;
               	       p_dec_curr -- ;
               	       out_len --;               	       
               	   }
               	   if( p3 == NULL )
               	   {
               	       p3 = p_curr;	
               	       continue;
               	   }
               }     
           }
           else
           {	
               /* 转义字符 */
               if( *p_curr == '\\' )
               {
                   i++;
                   p_curr++;
                   continue;
               }
           }

           /* 数组 */
           if (obj_type == 0 ) 
           {            
               if( in_value_flag < 0)
               {
                   /* 语法部分忽略空格和TAB */
                   if( (*p_curr == ' ') || 
               	       (*p_curr == '	') ||
               	       (*p_curr == '\r') ||
               	       (*p_curr == '\n')
               	     )
                   {
                       continue;
                   }               	
                   if( p3 == NULL )
                   {
                       p3 = p_curr;
                       continue;
                   }	
                   /* 出现{，是对象 */
                   if( *p_curr == '{' )
                   {
           	           p3 = NULL;
           	           p5 = NULL;
           	           obj_type = -1;
           	           data_type = -1;
           	           in_value_flag = -1;
           	           
           	           mode = 1;                   
                       continue;
                   }
                   /* 出现[，是数组 */
                   if( *p_curr == '[' )
                   {
           	           p5 = NULL;
           	           data_type = -1;
        	              	                              	
                       continue;
                   }
                   /* 字符串 */
                   else if( *p_curr == '\"' )
                   {
                       in_value_flag = 1;
                       data_type = 0;
                       if( p_dec != NULL )
                       {
                           cp_flag = -1;
                       }                       
                       if( p5 == NULL )
                       {
                           p5 = p_curr; 
                           continue;
                       }
                   }
                   /* 数组结束 */
                   else if( *p_curr == ']' )
                   {
           	           p3 = NULL;
           	           p5 = NULL;
           	           obj_type = -1;
           	           data_type = -1;
           	           in_value_flag = -1;
           	           mode = 2;
           	           continue;
                   }
                   /* 其它类型 */
                   else
                   {
                       in_value_flag = 1;
                       data_type = 1;
                       if( p_dec != NULL )
                       {
                           cp_flag = -1;
                           p_dec_curr -- ;
                           out_len --;                     
                       }

                       if( p5 == NULL )
                       {
                           p5 = p_curr; 
                           continue;
                       }                                                
                   }
               }
               /* 找到字符串结尾 */
               if( ( data_type == 0 ) &&
               	   ( *p_curr == '\"' )
               	 )
               {
               	   cp_flag = 1;
               	   in_value_flag = -1;
               	   dec_off_set = 0;
                   if( p_dec != NULL )
                   {
                       if ( (*p_dec_func)( p5+1, p_curr-p5-1, p_dec_curr, &dec_off_set ) != 0 )
                       {
                           printf("dec_func error\n");
                           return -1;
                       }                
                       p_dec_curr = p_dec_curr + dec_off_set;
                       *p_dec_curr = *p_curr;
                       p_dec_curr++;
                       out_len = out_len + dec_off_set + 1;
                   }
 
                   p5 = NULL;
                   continue;                
               }
               /* 字符串,结束 */
               else  if ( (data_type==0) && (*p_curr == ',') ) 
               {
                   in_value_flag = -1;
                   p5 = NULL;
                   continue;
               }               
               /* 其它类型,结束 */
               else if ( (data_type>0)  &&
                (
                  (*p_curr == ' ') || 
                  (*p_curr == '	') ||
                  (*p_curr == '\r') ||
                  (*p_curr == '\n') ||
                  (*p_curr == ']') ||
                  (*p_curr == ',')
                  ) 
               	) 
               {   
               	   cp_flag = 1; 

               	   
               	   if( p5 == NULL )
               	   {
               	       continue;
               	   }	
               	               	
               	   if( p_dec != NULL )
               	   {
                       if ( (*p_dec_func)( p5, p_curr-p5, p_dec_curr, &dec_off_set ) != 0 )
                       {
                           printf("dec_func error\n");
                           return -1;
                       }                
                       p_dec_curr = p_dec_curr + dec_off_set;
                       *p_dec_curr = *p_curr;
                       p_dec_curr++;
                       out_len = out_len + dec_off_set + 1;               	       
               	   }
            	  
               	   p5 = NULL;
               	   
               	   if( *p_curr == ',' )
               	   {
               	   	   in_value_flag = -1;
               	   	   p5 = NULL;  
               	       continue;            	       
               	   }
               	   
               	   if( *p_curr == ']' )
               	   {
               	   	   p3 = NULL;
               	   	   p5 = NULL;
               	       obj_type = -1;
               	       data_type = -1;
               	       in_value_flag = -1;
               	       mode = 2;   
               	       continue;            	       
               	   }               	     
                   continue;
               }
           }           	
           /* 是字符串 */
           else if( (obj_type == 1) && (*p_curr == '\"') )
           {
               dec_off_set = 0;
               if ( p3 == NULL  )
               {
                   p3 = p_curr;
                   continue;
               }
               cp_flag = 1;

               if( p_dec != NULL )
               {
                   if ( (*p_dec_func)( p3+1, p_curr-p3-1, p_dec_curr, &dec_off_set ) != 0 )
                   {
                       printf("dec_func error\n");
                       return -1;
                   }               
                   p_dec_curr = p_dec_curr + dec_off_set;
                   *p_dec_curr = *p_curr;
                   p_dec_curr++;
                   out_len = out_len + dec_off_set + 1 ; 
               }
          
               p3 = NULL;
               p5 = NULL;
               mode = 1;
               obj_type = -1;
               data_type = -1;
               in_value_flag = -1; 
               continue;
           }        
           /* 其它类型 */
           else if
           (  (obj_type==2) && 
              (
                  (*p_curr == ' ') || 
                  (*p_curr == '	') ||
                  (*p_curr == '\r') ||
                  (*p_curr == '\n') ||
                  (*p_curr == ']') ||
                  (*p_curr == '}') ||
                  (*p_curr == ',')
              ) 
           )
           {
               cp_flag = 1;     
               /* 处理*/
               if( p_dec != NULL )
               {
                   if ( (*p_dec_func)( p3, p_curr-p3, p_dec_curr, &dec_off_set ) != 0 )
                   {
                       printf("dec_func error\n");
                       return -1;
                   }                  	

                   p_dec_curr = p_dec_curr + dec_off_set;
                   *p_dec_curr = *p_curr;
                   p_dec_curr++;
                   out_len = out_len + dec_off_set + 1;                
               }
               
               p3 = NULL;
               p5 = NULL;               
               obj_type = -1;
               data_type = -1;
               in_value_flag = -1;
               mode = 1;               
               continue;
           }       
  
           continue;	    	
       }
       /* 其它部分 */
       else
       {
       	   continue;	
       }
   }
   
   return 0;
}


/*解密*/
int decode( int len )
{  
    char path[1024+1];
    char file_name[128+1];
    char wl_file[1024+1];
    char in_buf[10240+1];
    char out_buf[10240+1];   
    
    /*动态库相关*/
    void *handle;
    char *error;
    int out_len = 0;
    
    char *p1=NULL;
    char *p2=NULL;
    
    unsigned long file_size = -1;
    unsigned long read_size = 0;
    struct stat statbuff; 
    FILE *fp_in;

    memset( path, 0x00, sizeof(path) );
    memset( file_name, 0x00, sizeof(file_name) );
    memset( wl_file, 0x00, sizeof(wl_file) );
  
    /* 打开处理函数 */	
    handle = dlopen("./libdec_func.so", RTLD_LAZY);
    if (!handle) {
        printf("%s\n", dlerror());
        return -1;
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&p_dec_func) = dlsym(handle, "des_decode");
    if ((error = dlerror()) != NULL)  {
        printf("%s\n", error);
        return -1;
    }
 
    fp_in = fopen("in_buf.txt", "r");
    if(fp_in==NULL) 
    {
    	printf("打开文件[in_buf.txt]失败\n");
    	return -1;
    }
    fgets(in_buf, sizeof(in_buf),fp_in);

    p1 = strstr(in_buf,":");  printf("ssss[%s]\n",in_buf);
    p2 = strstr(p1,"\"");
    (*p_dec_func)( p2, len, out_buf, &out_len );
    
    fclose(fp_in);
    printf("\n\n解密后字符串 = [%s]\n", out_buf);

    //关闭动态链接库
    dlclose(handle);
           
    return 0;
}

/*打开动态库
 输入：
   lib_name 库文件名称 
   handle   指针
*/
int load_func( char *lib_name )
{ 
    /*动态库相关*/
    char *error;
    
    /* 打开处理函数 */	
    handle = dlopen(lib_name, RTLD_LAZY);
    if (!handle) {
        printf("%s\n", dlerror());
        return -1;
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&p_dec_func) = dlsym(handle, "dec_func");
    if ((error = dlerror()) != NULL)  {
        printf("%s\n", error);
        return -1;
    }    
            
    return 0;
}

/*关闭动态链接库*/
int free_handle( )
{
    return dlclose(handle);
}

/* DEMO
 * 1. 加载动态库
   2. 读入JSON报文,开辟空间
   3. 加载配置文件
 * 4. 处理函数调用dec_func.c里的dec_func()，原型如下
      int dec_func( const char* src, int src_len, char* dest, int *dest_len );

 * 注意事项：
   1. 如处理后数据比原数据长度长，输出的BUF要保证空间足够
*/
int main(int argc, char *argv[])
{  
    char path[1024+1];
    char file_name[128+1];
    char wl_file[1024+1];
    char buf[10240+1];
    char lib_name[1024+1];
    char tmp[512+1];
       
    char *p_in = NULL;
    char *p_out = NULL;
    
    unsigned long file_size = -1;
    unsigned long read_size = 0;
    struct stat statbuff; 
    FILE *fp;
    
    int ret=0;
    
    memset( path, 0x00, sizeof(path) );
    memset( file_name, 0x00, sizeof(file_name) );
    memset( wl_file, 0x00, sizeof(wl_file) );

    if( argv[1] == NULL || argv[2] == NULL)
    {
        printf("usage: %s json_file ./lib_name\n", argv[0]);
        return -1;
    }	
    strcpy( lib_name, argv[2]);


    /*调用解密,自测用*/
    if( argv[3] != NULL )
    {
        printf( "开始解密.长度[%d]\n", atoi(argv[2]) );
        return decode(atoi(argv[2]));
    }    
    
    /* 加载动态库 */
    if ( load_func( lib_name ) != 0 ) 
    {
        printf("load library fail\n");
        return -1;        
    }
    
    /* 读入JSON报文,开辟空间 */   
    getcwd(path,sizeof(path));
    sprintf(wl_file,"%s/%s",path,argv[1]); 
    if ( stat(wl_file, &statbuff) < 0 )
    {
        printf("stat error\n");
        free_handle();
        return -1;
    }	
    file_size = statbuff.st_size ;
    
    p_in = malloc(file_size*sizeof(char)); 
    memset( p_in, 0x00, file_size*sizeof(char) );
    /*空间要足够*/
    p_out = malloc(2*file_size*sizeof(char)); 
    memset( p_out, 0x00, 2*file_size*sizeof(char) );
    
    printf("start read json file \n");
    fp = fopen( wl_file, "r" );  
    if( !fp )
    {
        free(p_in);
        free(p_out); 
        free_handle();   	
        printf("read file error\n");
        return -1;    	
    }
    
    read_size = fread(p_in,1,file_size,fp);
    if(read_size!=file_size)
    {
        free(p_in);
        free(p_out); 
        free_handle();   	
        printf("read file size error:[%ld],[%ld]\n",read_size,file_size);
        return -1;      	
    }	
    fclose(fp);

    printf("read json file done\n");    
     
    /* 配置文件 */
    strcat(path,"/dec.ini");
    if( load_config(path) != 0 )
    {
        free(p_in);
        free(p_out); 
        free_handle();    	
        return -1;
    }
    
    /*可以直接赋值*/
    //sprintf(dec_ele,"\nurl\nname\n");
    
    /*调用处理*/   
    if( dec_data( p_in, file_size, p_out, 2*file_size ) != 0 )
    {
        free(p_in);
        free(p_out);  
        free_handle();   	
        return -1;
    } 

    /*打印结果*/
    if(file_size<51200)
    {	
        FILE *fp_out;
        fp_out = fopen("out_buf.txt", "w+");
        if(fp_out==NULL) return -1;
        fprintf(fp_out,p_out);
        fclose(fp_out);
        printf("\n\nnew buf = [%s]\n\n\n", p_out);
        
        //sprintf(tmp,"echo old buf=;cat %s", wl_file );
        system(tmp);
    }
    
    free(p_in);
    free(p_out);
    free_handle();
         
    printf("\nprocess complete\n");

    return 0;
}

