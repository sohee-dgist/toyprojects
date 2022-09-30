#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h> //atoi
#include <math.h> //log2
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#define bitsize 36

int L1_capacity;
int L2_capacity;
int L1_associativity;
int L2_associativity;
int block_size;


int L1_indexbitnum;
int L2_indexbitnum;
int L1_tagbitnum;
int L2_tagbitnum;
int L1_block_bit;
int L2_block_bit;

typedef struct Cblock{
    int tag;
    int dirtybit;
    int timestamp;
    int valid;
}Cblock;

typedef struct Performance{
    int Total_accesses;
    int Read_accesses;
    int Write_accesses;
    int L1_Rmiss;
    int L2_Rmiss;
    int L1_Wmiss;
    int L2_Wmiss;
    int L1_cevict;
    int L2_cevict;
    int L1_devict;
    int L2_devict;
}Performance;

int get_tag(u_int64_t PA,int bitnum){
    int result = (int)(PA>>bitnum);
    return result;
}

int get_index(u_int64_t PA, int bitnum,int bitnum2){
    int result = (int)((PA<<(28+bitnum))>>(28+bitnum2+bitnum));
    return result;
}

int log2funct(int n){
    int result = (int)(log2(n));
    return result;
}

int main(int argc, char* argv[]){
    //set numbers
    Performance cur_perf = {0,0,0,0,0,0,0,0,0,0,0};
    L2_capacity = atoi(argv[2]);
    L1_capacity = L2_capacity/4;
    L2_associativity = atoi(argv[4]);
    int lru = 0;
    if(strcmp(argv[7],"-lru")==0){
        lru = 1;
        printf("lru\n");
    }
    if(L2_associativity<4){
        L1_associativity = L2_associativity;
    }
    else{
        L1_associativity = L2_associativity/4;
    }
    block_size = atoi(argv[6]);
    L2_block_bit = log2funct(block_size);
    L1_block_bit = log2funct(block_size);
    L2_indexbitnum = log2funct(((pow(2,10)*L2_capacity)/block_size)/L2_associativity);
    L1_indexbitnum = log2funct(((pow(2,10)*L1_capacity)/block_size)/L1_associativity);
    L2_tagbitnum = 36-L2_block_bit-L2_indexbitnum;
    L1_tagbitnum = 36-L1_block_bit-L1_indexbitnum;

    /*
    printf("L2 : %d, %d, %d\n", L2_block_bit, L2_indexbitnum, L2_tagbitnum);
    printf("L1 : %d, %d, %d\n", L1_block_bit, L1_indexbitnum, L1_tagbitnum);
    printf("L2 associativity %d\n", L2_associativity);
    printf("L1 associativity %d\n", L1_associativity);
    */
    
    /*
    u_int64_t PA = 0x3c8fed220;
    //calculate L2 indexbit, tagbit
    int L2_index = get_index(PA,L2_tagbitnum,L2_block_bit);
    int L2_tag = get_tag(PA,L2_indexbitnum+L2_block_bit);
    int L1_index = get_index(PA,L1_tagbitnum,L1_block_bit);
    int L1_tag = get_tag(PA,L1_indexbitnum+L1_block_bit);
    printf("L2 index, tag: %x, %x\n", L2_index, L2_tag);
    printf("L1 index, tag: %x, %x\n", L1_index, L1_tag);
    */

    Cblock L1[(int)pow(2,L1_indexbitnum)][L1_associativity];
    Cblock L2[(int)pow(2,L2_indexbitnum)][L2_associativity];

    u_int64_t PA;
    bool RW; //Read = 0, Write 1;
    bool L1_hit; //miss = 0, hit = 1;
    bool L2_hit;
    const int max = 1024;
    char line[max];
    char *pLine;
    FILE *fp = fopen(argv[8],"r");
    //emulate part
    while(!feof(fp)){
        pLine = fgets(line,max,fp);
        char *ptr = strtok(pLine, " ");
        if(ptr==NULL){
            goto END;
        }
        if(strcmp(ptr,"W")==0){
            RW = true;
        }
        if(strcmp(ptr,"R")==0){
            RW = false;
        }
        ptr = strtok(NULL, " ");
        PA = strtol(ptr,NULL,16);
        //printf("W : %d PA : %lx \n",RW,PA);

        cur_perf.Total_accesses += 1;
        //bit mask
        int L2_index = get_index(PA,L2_tagbitnum,L2_block_bit);
        int L2_tag = get_tag(PA,L2_indexbitnum+L2_block_bit);
        int L1_index = get_index(PA,L1_tagbitnum,L1_block_bit);
        int L1_tag = get_tag(PA,L1_indexbitnum+L1_block_bit);

        //find in L1
        L1_hit = false;
        for(int i=0;i<L1_associativity;i++){
            if(L1[L1_index][i].valid==1){
                L1[L1_index][i].timestamp += 1;
                if(L1[L1_index][i].tag == L1_tag){
                    L1_hit = true;
                    L1[L1_index][i].timestamp = 0;
                    if(RW==true){
                        L1[L1_index][i].dirtybit = 1;
                    }
                }
            }
            /*
            if(L1[L1_index][i].tag == L1_tag){
                L1_hit = true;
                L1[L1_index][i].timestamp = 0;
                if(RW==true){
                    L1[L1_index][i].dirtybit = 1;
                }
            }
            */
        }

        //find in L2
        L2_hit = false;
        for(int i=0;i<L2_associativity;i++){
            if(L2[L2_index][i].valid==1){
                L2[L2_index][i].timestamp += 1;
                if(L2[L2_index][i].tag == L2_tag){
                    L2_hit = true;
                    L2[L2_index][i].timestamp = 0;
                }
            }
            /*
            if(L2[L2_index][i].tag == L2_tag){
                L2_hit = true;
                L2[L2_index][i].timestamp = 0;
            }
            */
        }

        if(RW==true){
            cur_perf.Write_accesses +=1;
        }
        else{
            cur_perf.Read_accesses +=1;
        }
        
        if(lru){
            //miss handle
            if(L1_hit==false){
                //where to write?
                int L1_max = 0;
                int L1_writeaddr = 0;
                int L1_vacant = 0;

                for(int i=0;(i<L1_associativity)&&(L1_vacant==0);i++){
                    if(L1[L1_index][i].valid!=1){
                        L1_vacant = 1;
                        L1_writeaddr = i;
                    }
                    else{
                        if(L1_max<L1[L1_index][i].timestamp){
                            L1_max = L1[L1_index][i].timestamp;
                            L1_writeaddr = i;
                        }
                    }
                }

                //evict
                if(L1[L1_index][L1_writeaddr].dirtybit==1){
                    cur_perf.L1_devict += 1;
                    for(int i=0;i<L2_associativity;i++){
                        if(L2[L2_index][i].tag == L1[L1_index][L1_writeaddr].tag){
                            L2[L2_index][i].dirtybit = 1;
                        }
                    }
                }
                else{
                    if(L1_vacant == 0){
                        cur_perf.L1_cevict += 1;
                    }
                }

                //evict_access
                L1[L1_index][L1_writeaddr].tag = L1_tag;
                L1[L1_index][L1_writeaddr].timestamp = 0;
                L1[L1_index][L1_writeaddr].valid = 1;

                if(RW==true){
                    cur_perf.L1_Wmiss += 1;
                    L1[L1_index][L1_writeaddr].dirtybit = 1;
                }
                else{
                    cur_perf.L1_Rmiss += 1;
                    L1[L1_index][L1_writeaddr].dirtybit = 0;
                }

                if(L2_hit==false){
                    int L2_max = 0;
                    int L2_writeaddr = 0;
                    int L2_vacant = 0;
                    for(int i=0;(i<L2_associativity)&&(L2_vacant==0);i++){
                        if(L2[L2_index][i].valid!=1){
                            L2_vacant = 1;
                            L2_writeaddr = i;
                        }
                        else{
                            if(L2_max<L2[L2_index][i].timestamp){
                                L2_max = L2[L2_index][i].timestamp;
                                L2_writeaddr = i;
                            }
                        }
                    }

                    //define here
                    int minus = L2_indexbitnum-L1_indexbitnum;
                    int delete_L1_index = (L2_index<<(32-L1_indexbitnum))>>(32-L1_indexbitnum);
                    int delete_L1_tag = (L2[L2_index][L2_writeaddr].tag<<(minus))|(L2_index>>L1_indexbitnum);

                    //evict
                    if(L2_vacant == 0){
                        if(L2[L2_index][L2_writeaddr].dirtybit == 1){
                            cur_perf.L2_devict += 1;
                        }
                        else{
                            cur_perf.L2_cevict += 1;
                        }
                    }

                    for(int i=0;i<L1_associativity;i++){
                        if(L1[delete_L1_index][i].tag == delete_L1_tag){
                            if(L1[L1_index][i].dirtybit == 1){
                                cur_perf.L1_devict += 1;
                                //cur_perf.L2_devict += 1;
                            }
                            else{
                                if(L2_vacant == 0){
                                    cur_perf.L1_cevict += 1;
                                }
                            }
                            L1[L1_index][i].valid = 1;
                            L1[L1_index][i].tag = L1_tag;
                            L1[L1_index][i].dirtybit = 0;
                            if(RW==true){
                                L1[L1_index][i].dirtybit = 1;
                            }
                        }
                    }
                
                    //evict_access
                    L2[L2_index][L2_writeaddr].dirtybit = 0;
                    if(RW==true){
                        L2[L2_index][L2_writeaddr].dirtybit = 1;
                    }
                    L2[L2_index][L2_writeaddr].tag = L2_tag;
                    L2[L2_index][L2_writeaddr].timestamp = 0;
                    L2[L2_index][L2_writeaddr].valid = 1;
        
                    if(RW==true){
                        cur_perf.L2_Wmiss += 1;
                    }
                    else{
                        cur_perf.L2_Rmiss += 1;
                    }
                }
            }
        }
        else{
            //miss handle
            if(L1_hit==false){
                //where to write?
                int L1_max = 0;
                int L1_writeaddr = 0;
                int L1_vacant = 0;

                for(int i=0;(i<L1_associativity)&&(L1_vacant==0);i++){
                    if(L1[L1_index][i].valid!=1){
                        L1_vacant = 1;
                        L1_writeaddr = i;
                    }
                }

                if(L1_vacant==0){
                    int random = rand() % L1_associativity;
                    L1_writeaddr = random;
                }
                
                //evict
                if(L1[L1_index][L1_writeaddr].dirtybit==1){
                    cur_perf.L1_devict += 1;
                    for(int i=0;i<L2_associativity;i++){
                        if(L2[L2_index][i].tag == L1[L1_index][L1_writeaddr].tag){
                            L2[L2_index][i].dirtybit = 1;
                        }
                    }
                }
                else{
                    if(L1_vacant==0){
                        cur_perf.L1_cevict += 1;
                    }
                }

                //evict_access
                L1[L1_index][L1_writeaddr].tag = L1_tag;
                L1[L1_index][L1_writeaddr].timestamp = 0;
                L1[L1_index][L1_writeaddr].valid = 1;

                if(RW==true){
                    cur_perf.L1_Wmiss += 1;
                    L1[L1_index][L1_writeaddr].dirtybit = 1;
                }
                else{
                    cur_perf.L1_Rmiss += 1;
                    L1[L1_index][L1_writeaddr].dirtybit = 0;
                }
            }
                //end
            if(L1_hit==false&&L1_hit==false){
                if(L2_hit==false){
                    //printf("a");
                    int L2_max = 0;
                    int L2_writeaddr = 0;
                    int L2_vacant = 0;

                    for(int i=0;(i<L2_associativity)&&(L2_vacant==0);i++){
                        if(L2[L2_index][i].valid!=1){
                            L2_vacant = 1;
                            L2_writeaddr = i;
                        }
                    }

                    if(L2_vacant==0){
                        int random = rand() % L2_associativity;
                        L2_writeaddr = random;
                    }

                    //here
                    int minus = L2_indexbitnum-L1_indexbitnum;
                    int delete_L1_index = (L2_index<<(32-L1_indexbitnum))>>(32-L1_indexbitnum);
                    int delete_L1_tag = (L2[L2_index][L2_writeaddr].tag<<(minus))|(L2_index>>L1_indexbitnum);

                    //evict
                    if(L2_vacant == 0){
                        if(L2[L2_index][L2_writeaddr].dirtybit == 1){
                            cur_perf.L2_devict += 1;
                        }
                        else{
                            cur_perf.L2_cevict += 1;
                        }
                    }
                    for(int i=0;i<L1_associativity;i++){
                        if(L1[delete_L1_index][i].tag == delete_L1_tag){
                            if(L1[L1_index][i].dirtybit == 1){
                                cur_perf.L1_devict += 1;
                                //cur_perf.L2_devict += 1;
                            }
                            else{
                                if(L2_vacant == 0){
                                    cur_perf.L1_cevict += 1;
                                }
                            }
                            L1[L1_index][i].valid = 1;
                            L1[L1_index][i].tag = L1_tag;
                            L1[L1_index][i].dirtybit = 0;
                            if(RW==true){
                                L1[L1_index][i].dirtybit = 1;
                            }
                        }
                    }

                    L2[L2_index][L2_writeaddr].dirtybit=0;
                    L2[L2_index][L2_writeaddr].tag = L2_tag;
                    L2[L2_index][L2_writeaddr].timestamp = 0;
                    L2[L2_index][L2_writeaddr].valid = 1;
        
                    if(RW==true){
                        //cur_perf.L1_Wmiss += 1;
                        cur_perf.L2_Wmiss += 1;
                    }
                    else{
                        //cur_perf.L1_Rmiss += 1;
                        cur_perf.L2_Rmiss += 1;
                    }

                }
            }
        }
    }
END:
    fclose(fp);

    char *new = strtok(argv[8],".");
    char *name = malloc(sizeof(char) * 30);
    strcpy(name,"");
    strcat(name,new);
    strcat(name,"_");
    strcat(name,argv[2]);
    strcat(name,"_");
    strcat(name,argv[4]);
    strcat(name,"_");
    strcat(name,argv[6]);
    strcat(name,".out");
    printf("%s\n",name);

    FILE *fnew = fopen(name,"w");
    free(name);
    fprintf(fp, "-- General Stats --\n");
    fprintf(fp, "L1 Capacity: %d\n",L1_capacity);
    fprintf(fp, "L1 way: %d\n",L1_associativity);
    fprintf(fp, "L2 Capacity: %d\n",L2_capacity);
    fprintf(fp, "L2 way: %d\n",L2_associativity);
    fprintf(fp, "Block Size: %d\n",block_size);
    fprintf(fp, "Total accesses: %d\n",cur_perf.Total_accesses);
    fprintf(fp, "Read accesses: %d\n",cur_perf.Read_accesses);
    fprintf(fp, "Write accesses: %d\n",cur_perf.Write_accesses);
    fprintf(fp, "L1 Read misses:%d\n",cur_perf.L1_Rmiss);
    fprintf(fp, "L2 Read misses:%d\n",cur_perf.L2_Rmiss);
    fprintf(fp, "L1 Write misses:%d\n",cur_perf.L1_Wmiss);
    fprintf(fp, "L2 Write misses:%d\n",cur_perf.L2_Wmiss);
    fprintf(fp, "L1 Read miss rate: %d%%\n",(int)((float)cur_perf.L1_Rmiss*100/(float)cur_perf.Read_accesses));
    fprintf(fp, "L2 Read miss rate: %d%%\n",(int)((float)cur_perf.L2_Rmiss*100/(float)cur_perf.L1_Rmiss));
    fprintf(fp, "L1 Write miss rate: %d%%\n",(int)((float)cur_perf.L1_Wmiss*100/(float)cur_perf.Write_accesses));
    fprintf(fp, "L2 write miss rate: %d%%\n",(int)((float)cur_perf.L2_Wmiss*100/(float)cur_perf.L1_Wmiss));
    fprintf(fp, "L1 Clean eviction: %d\n",cur_perf.L1_cevict);
    fprintf(fp, "L2 clean eviction: %d\n",cur_perf.L2_cevict);
    fprintf(fp, "L1 dirty eviction: %d\n",cur_perf.L1_devict);
    fprintf(fp, "L2 dirty eviction: %d\n",cur_perf.L2_devict);
    fclose(fnew);
    
    /*
    printf("Total accesses: %d\n",cur_perf.Total_accesses);
    printf("Read accesses: %d\n",cur_perf.Read_accesses);
    printf("Write accesses: %d\n",cur_perf.Write_accesses);


    printf("L1 read miss rate: %d%% \n",(int)((float)cur_perf.L1_Rmiss*100/(float)cur_perf.Read_accesses));
    printf("L2 read miss rate: %d%% \n",(int)((float)cur_perf.L2_Rmiss*100/(float)cur_perf.L1_Rmiss));
    printf("L1 write miss rate: %d%% \n",(int)((float)cur_perf.L1_Wmiss*100/(float)cur_perf.Write_accesses));
    printf("L2 write miss rate: %d%% \n",(int)((float)cur_perf.L2_Wmiss*100/(float)cur_perf.L1_Wmiss));


    printf("L1 Read misses: %d\n",cur_perf.L1_Rmiss);
    printf("L2 Read misses: %d\n",cur_perf.L2_Rmiss);
    printf("L1 Write misses: %d\n",cur_perf.L1_Wmiss);
    printf("L2 Write misses: %d\n",cur_perf.L2_Wmiss);
    printf("L1 Clean eviction: %d\n",cur_perf.L1_cevict);
    printf("L2 Clean eviction: %d\n",cur_perf.L2_cevict);
    printf("L1 Dirty eviction: %d\n",cur_perf.L1_devict);
    printf("L2 Dirty eviction: %d\n",cur_perf.L2_devict);
    //write part
    */
    
}