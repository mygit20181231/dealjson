#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>  
#include <dlfcn.h>



/* ������������󳤶� */
#define MAX_BUF_LEN 4096
/* VALUE����� */
#define MAX_VALUE_LEN  51200
//����ָ��
typedef int (*DEC_FUNC)( const char*, int, char*, int * );

/*��Ҫ���������*/
char dec_ele[MAX_BUF_LEN+1];
void *handle = NULL;
DEC_FUNC p_dec_func = NULL; 

/*
 * config_file �����ļ�����Ҫȫ·��
 * ����ֵ��0 �ɹ�
           -1 ʧ��
*/
int load_config( char * config_file )
{
    FILE *fp = NULL;
    char buf[100+1];

    int len = 0;
    int total = 0;
    
    /* �������ݿ�ʼ */
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
        /* ����#,�ո�,TAB�ȿ�ͷ���� */
        if( (buf[0] == '#') || 
        	  (buf[0] == ' ') || 
        	  (buf[0] == '	') || 
        	  ((len=strlen(buf))<=0) ||
        	  (buf[0] == '\n')
          )
        {
            continue;
        }
        /* [1]��ʼ����Ҫ��������� */
        else if ( !strncmp("[1]",buf,3)  )
        {
            dec_flag = 1;
            dec_ele[0] = '\n';
            continue;
        }
        /* [2]��ʼ����Ҫ���ܵ����� */
        else if ( !strncmp("[2]",buf,3) )
        {
            dec_flag = 2;
            /*�ݲ�����*/
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
                /* �س���Ϊ�ָ��� */
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
   /* KEY��һ��˫����λ�� */
   const char *p1 = NULL;
   /* KEY�ڶ���˫����λ�� */
   const char *p2 = NULL;
   /* VALUE��һ��λ�� */
   const char *p3 = NULL;
   /* VALUE���鿪ʼλ�� */
   const char *p5 = NULL;
  
   /* ��ǰλ�� */
   const char *p_curr = NULL; 
   /* ����ǰλ�� */
   char *p_dec_curr = NULL;    
   /* �Ƿ���Ҫ���� */
   char *p_dec = NULL;      
        
   /* KEY���� */
   char key[200+1];
   /* VALUEֵ */
   char value[MAX_VALUE_LEN+1];   
   char tmp[512+1];
   
   /* buf���� */
   int len = 0;
   /* VALUE���� */
   int out_len = 0;

    
   /* ����״̬ 
      1 ����KEY 
      2 �����﷨
      3 ����ֵ
    */
   int mode = 0;
   /* ����KV������־ */
   int kv_flag = 0;
   /* VALUE������ģʽ��־ */
   int array_flag = -1;
   /* 0 String, ����0����  */   
   int data_type = -1;
   /* �Ƿ��ڽ���VALUE */
   int in_value_flag = -1;
   /* VALUE���������� */
   int obj_type = -1;   
   int ch_num = 0;
   int i = 0;
   /* �Ƿ��ڿ��� */
   int cp_flag = 1;
   /* ��������ݳ��� */
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
   	   
   	   /* ����KEY���� */
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
       /* �����﷨���� */
       else if ( mode == 2 )
       {
           /* �﷨���ֺ��Կո��TAB */
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
               /* ֤���ڶ������ź�ð���������ַ�,����KEY */
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
       /* ����VALUE */
       else if ( mode == 3 )
       {
           /* δ��ֵ�� */
           if( in_value_flag < 0 )
           {
               /* �﷨���ֺ��Կո��TAB */
               if( (*p_curr == ' ') || 
               	   (*p_curr == '	') ||
               	   (*p_curr == '\r') ||
               	   (*p_curr == '\n')
               	  )
               {
   	   	           continue;
               }
               /* �ָ��� */               
               else if ( *p_curr == ',' )  
               {
                   continue;
               }                 
               /* ���� */
               else if( (*p_curr == '[') ||  (*p_curr == ']') || (obj_type==0) )
               {
                   obj_type = 0;
               }
               /* �ַ��� */               
               else if ( *p_curr == '\"' )
               {
                   obj_type = 1;
                   in_value_flag = 1;
               	   if( p_dec != NULL )
               	   {
               	       cp_flag = -1;
               	   }                   
               }
               /* ���� */               
               else if ( *p_curr == '{' )
               {
                   p3 = NULL;
                   p5 = NULL;
                   obj_type = -1;
                   data_type = -1;
                   in_value_flag = -1;
           	           
                   mode = 1;   
               }                           
               /* ������������ */
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
               /* ת���ַ� */
               if( *p_curr == '\\' )
               {
                   i++;
                   p_curr++;
                   continue;
               }
           }

           /* ���� */
           if (obj_type == 0 ) 
           {            
               if( in_value_flag < 0)
               {
                   /* �﷨���ֺ��Կո��TAB */
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
                   /* ����{���Ƕ��� */
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
                   /* ����[�������� */
                   if( *p_curr == '[' )
                   {
           	           p5 = NULL;
           	           data_type = -1;
        	              	                              	
                       continue;
                   }
                   /* �ַ��� */
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
                   /* ������� */
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
                   /* �������� */
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
               /* �ҵ��ַ�����β */
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
               /* �ַ���,���� */
               else  if ( (data_type==0) && (*p_curr == ',') ) 
               {
                   in_value_flag = -1;
                   p5 = NULL;
                   continue;
               }               
               /* ��������,���� */
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
           /* ���ַ��� */
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
           /* �������� */
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
               /* ����*/
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
       /* �������� */
       else
       {
       	   continue;	
       }
   }
   
   return 0;
}


/*����*/
int decode( int len )
{  
    char path[1024+1];
    char file_name[128+1];
    char wl_file[1024+1];
    char in_buf[10240+1];
    char out_buf[10240+1];   
    
    /*��̬�����*/
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
  
    /* �򿪴����� */	
    handle = dlopen("./libdec_func.so", RTLD_LAZY);
    if (!handle) {
        printf("%s\n", dlerror());
        return -1;
    }

    //���֮ǰ���ڵĴ���
    dlerror();

    //��ȡһ������
    *(void **) (&p_dec_func) = dlsym(handle, "des_decode");
    if ((error = dlerror()) != NULL)  {
        printf("%s\n", error);
        return -1;
    }
 
    fp_in = fopen("in_buf.txt", "r");
    if(fp_in==NULL) 
    {
    	printf("���ļ�[in_buf.txt]ʧ��\n");
    	return -1;
    }
    fgets(in_buf, sizeof(in_buf),fp_in);

    p1 = strstr(in_buf,":");  printf("ssss[%s]\n",in_buf);
    p2 = strstr(p1,"\"");
    (*p_dec_func)( p2, len, out_buf, &out_len );
    
    fclose(fp_in);
    printf("\n\n���ܺ��ַ��� = [%s]\n", out_buf);

    //�رն�̬���ӿ�
    dlclose(handle);
           
    return 0;
}

/*�򿪶�̬��
 ���룺
   lib_name ���ļ����� 
   handle   ָ��
*/
int load_func( char *lib_name )
{ 
    /*��̬�����*/
    char *error;
    
    /* �򿪴����� */	
    handle = dlopen(lib_name, RTLD_LAZY);
    if (!handle) {
        printf("%s\n", dlerror());
        return -1;
    }

    //���֮ǰ���ڵĴ���
    dlerror();

    //��ȡһ������
    *(void **) (&p_dec_func) = dlsym(handle, "dec_func");
    if ((error = dlerror()) != NULL)  {
        printf("%s\n", error);
        return -1;
    }    
            
    return 0;
}

/*�رն�̬���ӿ�*/
int free_handle( )
{
    return dlclose(handle);
}

/* DEMO
 * 1. ���ض�̬��
   2. ����JSON����,���ٿռ�
   3. ���������ļ�
 * 4. ����������dec_func.c���dec_func()��ԭ������
      int dec_func( const char* src, int src_len, char* dest, int *dest_len );

 * ע�����
   1. �紦������ݱ�ԭ���ݳ��ȳ��������BUFҪ��֤�ռ��㹻
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


    /*���ý���,�Բ���*/
    if( argv[3] != NULL )
    {
        printf( "��ʼ����.����[%d]\n", atoi(argv[2]) );
        return decode(atoi(argv[2]));
    }    
    
    /* ���ض�̬�� */
    if ( load_func( lib_name ) != 0 ) 
    {
        printf("load library fail\n");
        return -1;        
    }
    
    /* ����JSON����,���ٿռ� */   
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
    /*�ռ�Ҫ�㹻*/
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
     
    /* �����ļ� */
    strcat(path,"/dec.ini");
    if( load_config(path) != 0 )
    {
        free(p_in);
        free(p_out); 
        free_handle();    	
        return -1;
    }
    
    /*����ֱ�Ӹ�ֵ*/
    //sprintf(dec_ele,"\nurl\nname\n");
    
    /*���ô���*/   
    if( dec_data( p_in, file_size, p_out, 2*file_size ) != 0 )
    {
        free(p_in);
        free(p_out);  
        free_handle();   	
        return -1;
    } 

    /*��ӡ���*/
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

