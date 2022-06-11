#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <string.h>

const int MEMORY_SIZE = 128;
const int PAGE_TABLE_SIZE = 256;
const int PAGE_SIZE = 256;
const int TLB_SIZE = 16;
const int OFFSET_SIZE = 8;

FILE *file;
FILE *correct;
FILE *binFile;

pthread_t t[16];

int address, bin[15], pageB[6], offsetB[7];

int page_table[256][2], memory[128][256], tlb[16][2];

int tlbHit = 0, pageFault = 0, fifo_mem = 0, fifo_tlb = 0;

int lru_mem[128], lru_tlb[16];
int  pageD, offsetD, value, frame = -1, option = 0, option1 = 0;

void addTlb(int f);
void update_table(int n, int f);
int addMemory(signed char b[]);
void backing_store_search();
void tlb_search();
int memory_search(int f, int off);
int table_search();
void* search(void * arg);

void initialize();
void dec_bin(int num);
int bin_dec(int arr[]);

int main(int argc, char **argv) {
  int count = 0, physical = 0;
  float page_rate,tlb_rate, pf, th;
 
  if (argc != 4) {
      printf("invalid command");
      return 0;
  }
  if (strcmp(argv[3],"lru")== 1 && strcmp(argv[3],"fifo")== 1){
    printf("invalid command");
    return 0;
  }
    if (strcmp(argv[2],"lru")== 1 && strcmp(argv[2],"fifo")== 1){
    printf("invalid command");
    return 0;
  }
  //inicializar arrays
  initialize();
  if (strcmp(argv[2],"lru")== 0){//menoria
    option = 1;
  }
  if (strcmp(argv[3],"lru")== 0){//tlb
    option1 = 1;
  }
  //ler arquivo com endereços
  file = fopen(argv[1], "r");
  while (fscanf(file, "%d", &address) != EOF){
    if (!file) {
      printf("Unable to open file");
      fclose(file);
 return 0;
    }
    if (address < 0 || address >= 65536){
      printf("%d invalid address", address);
      fclose(file);
      return 0;
    }
   
    dec_bin(address);
    pageD = bin_dec(pageB);
    offsetD = bin_dec(offsetB);
   
    tlb_search(); //threads buscam frame na tlb
    if (frame == -1){
      if (table_search() == -1){  
        pageFault++;//procurar no binario
        backing_store_search(pageD);
      }
        physical = (table_search())*256 + offsetD;
        addTlb(table_search());
       
        value = memory_search(page_table[pageD][0], offsetD);
    }
    else{
      tlbHit++;
      physical = (frame*256) + offsetD;
      value = memory_search(frame, offsetD);
    }
    frame = -1;
   
    correct = fopen("correct.txt", "a");
    fprintf(correct,"Virtual address: %d Physical address: %d Value: %d\n", address, physical, value);
    fclose(correct);
    count++;
  }
  fclose(file);
 
  th = tlbHit; //transformando inteiro em float
  pf = pageFault;
  page_rate = pf/count;
  tlb_rate = th/count;

  correct = fopen("correct.txt", "a");
  fprintf(correct,"Number of Translated Addresses = %d\n", count);
  fprintf(correct,"Page Faults: %d\n", pageFault);
  fprintf(correct,"Page Faults Rate: %.3f\n", page_rate);
  fprintf(correct,"TLB Hits: %d\n", tlbHit);
  fprintf(correct,"TLB Hits Rate: %.3f\n", tlb_rate);
  fclose(correct);
  return 0;
}

void dec_bin(int num) {
 
    int bin[15];
    int aux;

    for (aux = 15; aux >= 0; aux--) {
      if (num % 2 == 0) {
          bin[aux] = 0;
          num = num / 2;
      }
      else {
          bin[aux] = 1;
          num = num / 2;
      }
    }

    for (aux = 0; aux <= 15; aux++) {
      if (aux <= 7){
         pageB[aux] = bin[aux];
      }
      else{
        offsetB[aux-8] = bin[aux];
      }
    }
}

int bin_dec(int arr[]) {
int i, tam, novoValor = 0;

for (i = 7; i >= 0; i--) {
if (arr[i] == 1) {
novoValor += pow(2, 7-i);
}
}
 
return novoValor;
}

void backing_store_search(){
  signed char buffer[256];
  int f;
 
  binFile = fopen("BACKING_STORE.bin", "rb");
 
  if (!binFile) {
    printf("Unable to open file BACKING_STORE.bin");
    fclose(binFile);
return;
  }

fseek(binFile, 256*pageD, SEEK_SET);

fread(buffer, sizeof(signed char), 256, binFile);
fclose(binFile);

  f = addMemory(buffer);
  update_table(1, f);
}

int addMemory(signed char b[]){
 
  int replace = 0, oldest = 0;
  for(int i = 0; i < MEMORY_SIZE; i++){
    if (memory[i][0] == -256){
      if (option == 1){
        lru_mem[i] = 0;
        for (int c = 0; c < i; c++){
          lru_mem[c] = lru_mem[c] + 1; //incrementando em tds
        }
      }

      for(int j = 0; j < PAGE_SIZE; j++){
        memory[i][j] = b[j]; //adiciona offsets
      }

      return i;
    }
  }  
 
  if (option == 1){//lru replacement
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (lru_mem[i] > oldest){
          oldest= lru_mem[i];
          replace = i;
        }
    }
    update_table(-1, replace);
    for(int j = 0; j < PAGE_SIZE; j++){
      memory[replace][j] = b[j];
    }
   
    for (int j = 0; j < MEMORY_SIZE; j++){
      lru_mem[j] = lru_mem[j] + 1; //incrementando
    }
   
    lru_mem[replace] = 0;
    return replace;
  }
  else{//fifo replacement      
      update_table(-1, fifo_mem);
      for(int j = 0; j < PAGE_SIZE; j++){
          memory[fifo_mem][j] = b[j];
        }
      replace = fifo_mem;
      if (fifo_mem == 127){
        fifo_mem = 0;
      }
      else{
        fifo_mem++;
      }      
    return replace;
  }
}

void update_table(int n, int f){
  if (n == 1){
      page_table[pageD][0] = f;
      page_table[pageD][1] = n;
    }
  else{
    for(int i = 0; i < PAGE_TABLE_SIZE; i++){
      if (page_table[i][0] == f){
        page_table[i][1] = n;
        // page nao esta mais nesse index da mem
      }
    }
  }
 
}

void addTlb(int f){
  int check = 0, oldest = 0, replace;
  for(int i = 0; i < TLB_SIZE; i++){
    if (tlb[i][0] == pageD){
      if (option1 == 1){
      for (int c = 0; c < TLB_SIZE; c++){
            lru_tlb[c] = lru_tlb[c] + 1; //incrementando em tds
        }
      lru_tlb[i] = 0;
      }
      check++; //checa se ja ta na tlb
      break;
    }
  }
  if (check == 0){
   
    if (option1 == 1){
      //lru replacement
      for (int i = 0; i < TLB_SIZE; i++){
          if (lru_tlb[i] > oldest){
            oldest= lru_tlb[i];
            replace = i;
          }
      }
      for (int j = 0; j < MEMORY_SIZE; j++){
        lru_tlb[j] = lru_tlb[j] + 1; //incrementando
      }
     
      lru_tlb[replace] = 0;
      tlb[replace][0]= pageD;
      tlb[replace][1]= f;
    }
    else{
      //fifo replacement
      tlb[fifo_tlb][0]= pageD;
      tlb[fifo_tlb][1]= f;
     
      if (fifo_tlb == 15){
        fifo_tlb = 0;
      }
      else{
        fifo_tlb++;
      }
    }
  }
}

int memory_search(int f, int off){
  for(int i = 0; i < MEMORY_SIZE; i++){
    if (i == f){
      if(option == 1){
        for (int c = 0; c < MEMORY_SIZE; c++){
            lru_mem[c] = lru_mem[c] + 1; //incrementando em tds
        }
        lru_mem[i] = 0;
      }
      for(int j = 0; j < PAGE_SIZE; j++){
        if (j == off){
       
          addTlb(memory[i][0]);
          return memory[i][j];
        }
      }
    }
  }
  return 0;
}

int table_search(){
  for(int i = 0; i < PAGE_TABLE_SIZE; i++){
    if (i == pageD){
      //pagina encontrada
      if(page_table[i][1] == 1){
        //pagina ja está alocada na memoria
        return page_table[i][0];
      }
      else{// nao está na memoria
        return -1;
      }
    }
  }
  return -1;
}

void tlb_search(){
    int i, a;
  for (int i = 0; i < 16; i++) {
    int* a = malloc(sizeof(int));
    *a = i;
    pthread_create (&t[i], NULL, search, a);
  }
  for (int i = 0; i < 16; i++) {
    pthread_join (t[i], NULL);
  }
}

void* search(void * arg){
  int index = *(int*)arg;
 
  //cada thread checa um indice da tlb
  if (tlb[index][0] == pageD){
    if (option1 == 1){
      for (int j = 0; j < TLB_SIZE; j++){
        lru_tlb[j] = lru_tlb[j] + 1; //incrementando
      }
     
      lru_tlb[index] = 0;
    }
    frame = tlb[index][1]; //retorna frame
  }
  free(arg);
  return arg;
}

void initialize(){
  for(int i = 0; i < PAGE_TABLE_SIZE; i++){
    page_table[i][1] = -1; //bit inativo
  }

  for(int i = 0; i < MEMORY_SIZE; i++){
    memory[i][0] = -256;//pode ser que o primeiro offset seja -1
  }
 
  if (option == 1){
    for (int i = 0; i < MEMORY_SIZE; i++){
      lru_mem[i] = -1;
    }
  }
  if (option1 == 1){
    for (int i = 0; i < TLB_SIZE; i++){
      lru_tlb[i] = -1;
    }
  }
}
