#define my_annotation __attribute__((annotate("my_annotation")))

void nullcall(int * s);
char* swap(char **p, char **q){
 
       if(p){
         // *p=39;
         p=q;
          
       }
       char* a=*p;
      return a;
}

char* swap2(char **p, char **q){
 
       if(p){
         *p=*q;
         
          
       }
       char* a=*p;
      return a;
}

void go(){

   char* d,e,c;
   if(c){
      // d=84;
      swap(&d,&e);
   }
   
}
__attribute__((annotate("mfwrfannotation")))
void s(){
    char* d,e,c;
   if(c){
      // d=84;
      swap(&d,&e);
   }else{
      go();
   }
   
}

int main(){
      char a1, b1; 
      char cww=a1+b1;__attribute__((annotate("fgreger")))
      // char ff=cww+a1+12344;
      char *a = &a1;
      char *b = &b1;
      char* (*ss)(char**,char**)=&swap;
      go();
      
     int e my_annotation;
     if(e){
      // a=54;
      ss=&swap2;
      s();
     }
     if(b1){
      char *p=ss(&a,&b);
     }else{
      swap(&b,&a);
     }
      char* g my_annotation =a;
      

}
