/*
 * Copyright 2021 cpu-dev
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>

//#define DEBUG

#define MODE_BIN 0
#define MODE_VERILOG 1

struct label {
    char name;
    uint8_t addr;
    struct label *next;
};

struct label *head;

typedef enum {
    MOV = 0,
    ADD,
    AND = 3,
    OR,
    NOT,
    SLL,
    SRL,
    SRA,
    CMP,
    JE,
    JMP,
    LDIH,
    LDIL,
    LD,
    ST,
    IRET,
    LDI
} inst_t;

typedef enum {
    R0,
    R1,
    R2,
    R3
} reg_t;

int8_t
opcode(char *mnemonic)
{
    if(!strncmp(mnemonic, "mov", 3))
        return MOV;
    if(!strncmp(mnemonic, "add", 3))
        return ADD;
    if(!strncmp(mnemonic, "and", 3))
        return AND;
    if(!strncmp(mnemonic, "or", 2))
        return OR;
    if(!strncmp(mnemonic, "not", 3))
        return NOT;
    if(!strncmp(mnemonic, "sll", 3))
        return SLL;
    if(!strncmp(mnemonic, "srl", 3))
        return SRL;
    if(!strncmp(mnemonic, "sra", 3))
        return SRA;
    if(!strncmp(mnemonic, "cmp", 3))
        return CMP;
    if(!strncmp(mnemonic, "je", 2))
        return JE;
    if(!strncmp(mnemonic, "jmp", 3))
        return JMP;
    if(!strncmp(mnemonic, "ldih", 4))
        return LDIH;
    if(!strncmp(mnemonic, "ldil", 4))
        return LDIL;
    if(!strncmp(mnemonic, "ldi", 3))
        return LDI;
    if(!strncmp(mnemonic, "ld", 2))
        return LD;
    if(!strncmp(mnemonic, "st", 2))
        return ST;
    if(!strncmp(mnemonic, "iret", 4))
        return IRET;
    return -1;
}

void
new_label(char name, int addr)
{
    struct label *p;
    p = head;

    while(p->next != NULL)
        p = p->next;

    p->next = (struct label *)malloc(sizeof(struct label));
    p->next->name = name;
    p->next->addr = addr;
    p->next->next = NULL;
}

uint8_t
search_label(char name)
{
    struct label *p;
    p = head->next;

    while(p != NULL) {
        if(p->name == name) {
            return p->addr;
        }
        p = p->next;
    }
    printf("unknown label referenced: %c\n", name);
    exit(-1);
}

uint8_t
rs(char *p)
{
#ifdef DEBUG
    printf("arguments(rs): %s\n", p);
#endif
    return strtol(++p, &p, 16);
}

uint8_t
imm(char *p)
{
#ifdef DEBUG
    printf("arguments(imm): %s\n", p);
#endif
    return strtol(p, &p, 16);
}

uint8_t
ex_imm(char *p)
{
#ifdef DEBUG
    printf("arguments(ex_imm): %s\n", p);
#endif
    if(isalpha(*p))
        return search_label(*p);
    return strtol(p, &p, 16);
}

uint8_t
rdrs(char *p)
{
#ifdef DEBUG
    printf("arguments(rd/rs): %s\n", p);
#endif
    int rd, rs;
    rd = strtol(++p, &p, 16);
    while(isspace(*p) || *p == ',')
        ++p;
    rs = strtol(++p, &p, 16);
    return (rd << 2) + rs;
}

int
main(int argc, char *argv[])
{
    char mode = MODE_BIN;
    char buf[256];
    char *p;
    int8_t inst, result;
    int i;
    FILE *in, *out;

    if((in = fopen(argv[1], "r")) == NULL) {
        puts("file open error!");
        return -1;
    }

    if(argc > 2) {
        if(strncmp(argv[2], "verilog", 7) == 0)
            mode = MODE_VERILOG;
    }

    if(mode == MODE_BIN) {
        if((out = fopen("out.bin", "wb")) == NULL) {
            puts("file open error!");
            return -1;
        }
    }
    else {
        if((out = fopen("out.txt", "w")) == NULL) {
            puts("file open error!");
            return -1;
        }
    }

    head = (struct label *)malloc(sizeof(struct label));
    head->name = 0;
    head->addr = 0;
    head->next = NULL;

    i = 0;
    while(fgets(buf, 256, in) != NULL) {
        if(*buf == '\n') continue;
        p = buf;
        while(isspace(*p)) ++p;
        if(!strncmp(p, "//", 2)) continue;
        inst = opcode(p);
        while(isalpha(*p)) ++p;
        if(*p == ':') {
#ifdef DEBUG
            printf("new label in L%d: %c\n", i, *(p-1));
#endif
            new_label(*(p-1), i);
        }
        else {
            if(inst == LDI) ++i;
            ++i;
            continue;
        }
    }

    rewind(in);
    i = 0;
    while(fgets(buf, 256, in) != NULL) {
        if(*buf == '\n') continue;
        p = buf;
        while(isspace(*p)) ++p;
        if(!strncmp(p, "//", 2)) continue;
        inst = opcode(p);
#ifdef DEBUG
        printf("instruction number: %d\n", inst);
#endif
        while(isalpha(*p)) ++p;
        if(*p == ':') continue;
        while(isspace(*p)) ++p;
        switch(inst) {
            case -1:
                printf("invalid instruction!!!\n");
                return -1;
            case JE:
            case JMP:
                result = (inst << 4) | rs(p);
                mode == MODE_BIN 
                    ? fwrite(&result, 1, 1, out) 
                    : fprintf(out, "mem[%d] = 0x%02x;\n", i++, result & 0x000000ff);
                break;
            case LDIH:
            case LDIL:
                result = (inst << 4) | imm(p);
                mode == MODE_BIN 
                    ? fwrite(&result, 1, 1, out) 
                    : fprintf(out, "mem[%d] = 0x%02x;\n", i++, result & 0x000000ff);
                break;
            case IRET:
                result = 0xb4;
                mode == MODE_BIN
                    ? fwrite(&result, 1, 1, out)
                    : fprintf(out, "mem[%d] = 0x%02x;\n", i++, result & 0x000000ff);
                break;
            case LDI:
                result = (LDIH << 4) | (ex_imm(p) >> 4);
                mode == MODE_BIN
                    ? fwrite(&result, 1, 1, out)
                    : fprintf(out, "mem[%d] = 0x%02x;\n", i++, result & 0x000000ff);
                result = (LDIL << 4) | (ex_imm(p) & 0x0f);
                mode == MODE_BIN
                    ? fwrite(&result, 1, 1, out)
                    : fprintf(out, "mem[%d] = 0x%02x;\n", i++, result & 0x000000ff);
                break;
            default:
                result = (inst << 4) | rdrs(p);
                mode == MODE_BIN 
                    ? fwrite(&result, 1, 1, out) 
                    : fprintf(out, "mem[%d] = 0x%02x;\n", i++, result & 0x000000ff);
                break;
        }
    }
    return 0;
}

