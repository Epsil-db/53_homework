#include "linkedList.h"
#include "icssh.h"
/*
    What is a linked list?
    A linked list is a set of dynamically allocated nodes, arranged in
    such a way that each node contains one value and one pointer.
    The pointer always points to the next member of the list.
    If the pointer is NULL, then it is the last node in the list.

    A linked list is held using a local pointer variable which
    points to the first item of the list. If that pointer is also NULL,
    then the list is considered to be empty.
    -------------------------------               ------------------------------              ------------------------------
    |HEAD                         |             \ |              |             |            \ |              |             |
    |                             |-------------- |     DATA     |     NEXT    |--------------|     DATA     |     NEXT    |
    |-----------------------------|             / |              |             |            / |              |             |
    |LENGTH                       |               ------------------------------              ------------------------------
    |COMPARATOR                   |
    |PRINTER                      |
    |DELETER                      |
    -------------------------------

*/

List_t* createList(int (*compare)(const void*, const void*)) {
    List_t* list = malloc(sizeof(List_t));
    list->comparator = compare;
    list->head = NULL;
    list->length = 0;

}

void insertFront(List_t* list, void* valref) {
    if (list->length == 0)
        list->head = NULL;

    node_t** head = &(list->head);
    node_t* new_node;
    new_node = malloc(sizeof(node_t));

    new_node->value = valref;

    new_node->next = *head;
    *head = new_node;
    list->length++; 
}

void insertInOrder(List_t* list, void* valref) {
    if (list->length == 0) {
        insertFront(list, valref);
        return;
    }
    node_t** head = &(list->head);
    node_t* new_node;
    new_node = malloc(sizeof(node_t));
    new_node->value = valref;
    new_node->next = NULL;
    if (list->comparator(new_node->value, (*head)->value) <= 0) {
        new_node->next = *head;
        *head = new_node;
    } 
    else if ((*head)->next == NULL){ 
        (*head)->next = new_node;
    }                                
    else {
        node_t* prev = *head;
        node_t* current = prev->next;
        while (current != NULL) {
            if (list->comparator(new_node->value, current->value) > 0) {
                if (current->next != NULL) {
                    prev = current;
                    current = current->next;
                } else {
                    current->next = new_node;
                    break;
                }
            } else {
                prev->next = new_node;
                new_node->next = current;
                break;
            }
        }
        prev = NULL;
        current = NULL;
    }
    list->length++;
    head = NULL;
    new_node = NULL;
}

void* removeFront(List_t* list) {
    node_t** head = &(list->head);
    void* retval = NULL;
    node_t* next_node = NULL;

    if (list->length == 0) {
        return NULL;
    }
    bgentry_t* toDel = (bgentry_t*) (*head)->value;

    free_job(toDel->job);
    free(toDel);
    toDel = NULL;

    next_node = (*head)->next;
    retval = (*head)->value;
    list->length--;

    node_t* temp = *head;
    *head = next_node;
    free(temp);
    temp = NULL;

    head = NULL;
    next_node = NULL;

    return retval;
}

void removeByPID(List_t* list, int pid) {
    node_t** head = &(list->head);
    node_t* current = *head;
    bgentry_t* currEntry = (bgentry_t*)current->value;

    if (list->length == 0) {
        currEntry = NULL;
        return;

    } else if (list->length == 1) {
        list->head = NULL;
        list->length = 0;
    } else {
        if(currEntry->pid == pid) {
            node_t* next_node = (*head)->next;
            *head = next_node;
            head = NULL;
        } else {
            node_t* prev = NULL;
            int deleted = 0;

            while(current != NULL && !deleted) {
                prev = current;
                current = current->next;
                currEntry = (bgentry_t*)current->value;
                if(currEntry->pid == pid) {
                    prev->next = current->next;
                    current->next = NULL;
                    list->length--;
                    deleted = 1;
                }
            }
        }
    }
    
    free(current);
    printf(BG_TERM, pid, currEntry->job->line);
    free_job(currEntry->job);
    free(currEntry);
}

void deleteList(List_t** list) {

    if ((*list)->length == 0)
        return;
    while ((*list)->head != NULL){
        bgentry_t* currEntry = (bgentry_t*)(*list)->head->value;
        printf(BG_TERM, currEntry->pid, currEntry->job->line);
        kill(currEntry->pid, SIGKILL);
        removeFront(*list);
    }
}

int bgentryComparator(const  void* bg1, const void* bg2) {
    bgentry_t* b1 = (bgentry_t*) bg1;
    bgentry_t * b2 = (bgentry_t*) bg2;
    if (b1->seconds < b2->seconds) {
        return -1;
    } else if (b1->seconds > b2->seconds) {
        return 1;
    }
    return 0;
}

bgentry_t* createBGEntry(job_info *job, pid_t pid, time_t seconds) {
    if (job == NULL)
        return NULL;

    bgentry_t* newBG = malloc(sizeof(bgentry_t));
    newBG->job = job;
    newBG->pid = pid;
    newBG->seconds = seconds;

    return newBG;
}

void printList(List_t* list, char mode) {
    node_t* head = list->head;
    while(head != NULL) {
        print_bgentry(head->value);
        head = head->next;
    }
}


void printAscii() {
    char * petr = 
"                       Its supposed to be Petr haha                             \n" 
"................................................................................\n"
".........................::.....................................................\n"
"......................:F$MNF..................::**::............................\n"
"....................:VN$VVNV.............:*I$MMM$$NN*...........................\n"
"..................:FM$I**$NF.........:FVMM$VIF**I$NF............................\n"
".................*$MVF***$N*.......:VN$VF******VMM*.............................\n"
"...............:VN$F*FII*$N*......*MMI**FV$$F*$NV:..............................\n"
"...............$N$F*F$NM*VNV......MNVV$MMNN$*VNV................................\n"
"..............*N$FFVMMNM*FNM....:*V$$N$*VN$FFMM:................................\n"
"..............*N$VMMF:MNF*$N*.*VNNMMMMVFN$FFMN*.................................\n"
"...............F$$F:..INV*VMVVN$$MVVVF*VNV*VN$..................................\n"
"....................:*F$$***FVI**FVV$VVFF**FIF**:...............................\n"
"..................:V$MMMM$VF****VMMVII$MMVF*******:.............................\n"
".................*M$V****VN$****MNF::*$VFNM****FF**.............................\n"
".................$NI$F:*FMMI****VMM$$V$V$N$****$MI..............................\n"
"................:F$MMMMMM$VVV$$$I*FFIVVVVI*****FVM$:............................\n"
"...............:FIVV$MMMM$$$VVVVF****************IMM*..........:*****::.........\n"
"...........:*FVMMM$$VIFF************************V$$NN:......:IMM$$$$$MM$I*......\n"
".......:*VMM$VVIF*******************************$NMF*......:MMVF******FI$MM*....\n"
".....:FMMVI**************************************VM$:.....:MMI***********F$NV...\n"
"....*MMVF*************************************FVVV$N$.....VNV**************$N*..\n"
"...FN$F********************FFIVV$$V***********VN$FFF*....*N$***************FNM..\n"
"..IN$F**************FIVV$$MMMM$$VVF***********$NV********MMF****************$N*.\n"
".:NMF********FIVV$$MNM$$VVIFF*****************F$$$$$$$$$MN$F****************VNV.\n"
"..$NVFFIVV$$MNNNMM$$$F*************************VVF*FFFF*FV$N$I**************FN$.\n"
"...*$MM$MNNM$VIF***V$F*************************VMMMMNNNM$F*I$N$V************FNM.\n"
"......:VN$I**IVV$$$MMF**************************FMM$NMVVMM$F*F$N$V**********FNM.\n"
".....*MMI*FVMMVF**$NV********************FFIVV$$$MNNVF***V$N$F*I$N$F********IN$.\n"
"....*MMF*VM$*:....$NNNNNNNNNNNNNNNNNNNNNNNNNNNN$VF$NV******VMMVF*IMMV*******$N*.\n"
"...:MMF*$NI......:NMFIV$MNNNNNNNNNNNNNNNNNM$I*****VN$*******FVMMI*F$N$*****IN$..\n"
"...*N$*IN$.......*N$*******IV$MNNNNNNNM$I*********VN$*********IMMI*F$NV****$N*..\n"
"...VNV*$NF.......VNV*************FFIF*************VN$**********VNMF*FMNF**IN$...\n"
"..:$NVVMN:.......$NI******************************VN$*********FINN$$$MN$$VMN*...\n"
".FMM$$$$MM:......$NI******************************$NV********IMM$$VVVVV$NNNV....\n"
".IN$$$$MNN*......$NI******************************MNI********VNM$$$$MMMNNN$.....\n"
".*N$FF**INV......INV*****************************FNMF*****IVF*$NNF*****$NV......\n"
".FNV*:**VNI......:NMF****************************INM*F$$*VMMFVNNM*::::*$N*......\n"
".:V$MMM$V*........*N$F***************************VN$VMMVVNMFIMM$NVFFV$M$*.......\n"
"...................*MMV*************************FMNMMVI$NV*VM$::*IIIF*:.........\n"
"....................:$NMVF*********************IMNV*:.*V*..**...................\n"
"....................VN$VMM$VIF***************I$M$*..............................\n"
"..................:$NV**F$NMMM$$VVIFFFFFFIV$MMV*................................\n"
".................:$MV**IMM*:.:**IV$MMMMMMNM$MM*.................................\n"
"................:$NV**VM$:...........::.:MMFF$NF................................\n"
"................VN$**IMM:................*NMF*$N*...............................\n"
"................MNI*FMM:..................VNV*IN$...............................\n"
"...............*NM**$NF...................FNV*FMN:..............................\n"
"...............$NV**MN:...................*N$**MN*..............................\n"
"..............:NMF*FNM....................*N$**$N*..............................\n"
"..............:NM**IN$....................*NMVVMN*..............................\n"
"..............*NMVV$NV...................*N$VVV$NV:.............................\n"
"............:FNM$$$$MM:..................*NMVFFFMNI:::::::......................\n"
"......*FV$$MMMMMMMMMMN*..................FN$VV$$$$MMMMM$$MM*....................\n"
"....:$MVVIFF*******F$N*..................*N$**************MN:...................\n"
"....VNV************FMM:..................:MMVIFF********FIMM:...................\n"
"....:VM$VVVVVVVVVV$MM*....................:*IV$MMMMM$MMMM$I:....................\n"
"......:*FIVVVIIFF***:...........................::::::::........................\n"
"................................................................................\n";
printf("%s", petr);
}
