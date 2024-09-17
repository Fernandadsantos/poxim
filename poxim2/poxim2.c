#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define ZN (1 << 6)
#define CY (1 << 0)
#define OV (1 << 3)
#define SN (1 << 4)
#define ZD (1 << 5)
#define IV (1 << 2)

#define CR 26
#define IPC 27
#define IR 28
#define PC 29
#define SP 30
#define SR 31

#define COUNTER 0x80808080
#define TERMINAL 0x88888888
#define IN 0x8888888A
#define OUT 0x8888888B
#define X 0x80808880
#define x_ 0x20202220
#define Y 0x80808884
#define y_ 0x20202221
#define Z 0x80808888
#define z_ 0x20202222
#define st 0x8080888C
#define op 0x8080888F

typedef struct Terminal
{
    int size;
    char *buffer;
} Terminal;

typedef struct Watchdogs
{
    int watchdogState;
    uint32_t counter;
    int en;
} Watchdogs;

typedef struct FPU
{
    float z_float, x_float, y_float;
    uint32_t OP_ST, x, y, z;
} FPU;

int interruptionState = 0;
int causeOfInterruption = 0;
int pendente = 0;
int operacaoPendente;
int ciclo = 0;

void ISR(uint32_t *MEM32, uint32_t *R)
{
    MEM32[R[SP]] = R[PC] + 4;
    R[SP] -= 4;

    MEM32[R[SP]] = R[CR];
    R[SP] -= 4;

    MEM32[R[SP]] = R[IPC];
    R[SP] -= 4;
}

void identifiesInterruption(uint8_t causeOfInterruption, uint32_t aux_int, uint32_t *R, FILE *output, char *instrucao, uint32_t *MEM32)
{
    switch (causeOfInterruption)
    {
    case 2:
        ISR(MEM32, R);
        fprintf(output, "[SOFTWARE INTERRUPTION]\n");
        R[31] |= IV;
        R[CR] = (R[IR] & (0b111111 << 26)) >> 26;
        R[IPC] = R[PC];
        R[PC] = 0x00000004;
        R[PC] -= 4;
        interruptionState = 0;

        break;
    case 3:
        // divisão por zero
        ISR(MEM32, R);
        fprintf(output, "[SOFTWARE INTERRUPTION]\n");
        R[CR] = 0;
        R[IPC] = R[PC];
        R[PC] = 0x00000008;
        R[PC] -= 4;
        interruptionState = 0;

        break;
    case 4:
        ISR(MEM32, R);
        R[CR] = aux_int;
        R[IPC] = R[PC];
        uint32_t currentPC = R[PC];
        R[PC] = 0x0000000C;
        fprintf(output, "0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08X\n", currentPC, instrucao, R[CR], R[PC]);
        fprintf(output, "[SOFTWARE INTERRUPTION]\n");
        R[PC] -= 4;
        interruptionState = 0;
        break;
    // watchdogs
    case 5:
        ISR(MEM32, R);
        R[CR] = 0xE1AC04DA;
        R[IPC] = R[PC] + 4;
        R[PC] = 0x00000010;
        R[PC] -= 4;
        fprintf(output, "[HARDWARE INTERRUPTION 1]\n");

        break;
    // Erro em operação
    case 6:
        ISR(MEM32, R);
        R[CR] = 0x01EEE754;
        R[IPC] = R[PC];
        R[PC] = 0x00000014;
        R[PC] -= 4;
        fprintf(output, "[HARDWARE INTERRUPTION 2]\n");

        break;
    // Operações com tempo variavel
    case 7:
        ISR(MEM32, R);
        R[CR] = 0x01EEE754;
        R[IPC] = R[PC];
        R[PC] = 0x00000018;
        R[PC] -= 4;
        fprintf(output, "[HARDWARE INTERRUPTION 3]\n");

        break;
    // Operações com tempo constante
    case 8:
        ISR(MEM32, R);
        R[CR] = 0x01EEE754;
        R[IPC] = R[PC];
        R[PC] = 0x0000001C;
        R[PC] -= 4;
        fprintf(output, "[HARDWARE INTERRUPTION 4]\n");

        break;

    default:
        break;
    }
}

uint8_t calc_ciclos(FPU *fpu)
{
    // Extrair os expoentes (bits 23-30) de x e y
    uint8_t exp_x = ((uint8_t)fpu->x_float & (0xFF << 23)) >> 23;
    uint8_t exp_y = ((uint8_t)fpu->y_float & (0xFF << 23)) >> 23;

    int ciclos;

    // Se for uma das operações que têm ciclos fixos
    uint32_t OP = fpu->OP_ST & 0x1F;
    if (OP == 0x05 || OP == 0x06 || OP == 0x07 ||
        OP == 0x08 || OP == 0x09)
    {
        ciclos = 1;
    }
    else
    {
        // Calcular a diferença dos expoentes para outras operações
        ciclos = abs(exp_x - exp_y) + 1;
    }

    return ciclos;
}

uint32_t ceil_custom(float num)
{
    int integer_part = (int)num;
    if (num > (uint32_t)integer_part)
    {
        return (uint32_t)(integer_part + 1);
    }
    else
    {
        return (uint32_t)integer_part;
    }
}

uint32_t floor_custom(float num)
{
    int integer_part = (int)num;

    if (num < (uint32_t)integer_part)
    {
        return (uint32_t)(integer_part - 1);
    }
    else
    {
        return (uint32_t)integer_part;
    }
}

uint32_t round_custom(float num)
{
    int integer_part = (int)num;
    float fraction_part = num - (float)integer_part;

    if (fraction_part >= 0.5)
    {
        return (uint32_t)(integer_part + 1);
    }
    else if (fraction_part <= -0.5)
    {
        return (uint32_t)(integer_part - 1);
    }
    else
    {
        return (uint32_t)integer_part;
    }
}

void calcFPU(FPU *fpu)
{
    uint32_t OP = fpu->OP_ST & 0x1F;
    switch (OP)
    {
    case 0b00001:
        // Adição Z = X + Y
        fpu->x_float = (float)fpu->x;
        fpu->y_float = (float)fpu->y;
        operacaoPendente = 3;
        fpu->z = fpu->x_float + fpu->y_float;
        break;
    case 0b00010:
        // Subtração Z = X − Y
        fpu->z = fpu->x - fpu->y;
        operacaoPendente = 3;
        break;
    case 0b00011:
        // Multiplicação Z = X × Y
        fpu->z = fpu->x * fpu->y;
        operacaoPendente = 3;
        break;
    case 0b00100:
        // Divisão Z = X ÷ Y
        if (fpu->y == 0)
        {
            fpu->OP_ST |= 0x20;
            operacaoPendente = 2;
        }
        else
        {
            fpu->z = fpu->x / fpu->y;
            operacaoPendente = 3;
        }

        break;
    case 0b00101:
        // Atribuição X = Z
        fpu->x = fpu->z;
        operacaoPendente = 4;
        break;
    case 0b00110:
        // Atribuição Y = Z
        fpu->y = fpu->z;
        operacaoPendente = 4;
        break;
    case 0b00111:
        // Teto ⌈Z ⌉
        operacaoPendente = 4;
        fpu->z = ceil_custom(fpu->z_float);
        break;
    case 0b01000:
        // Piso ⌊Z ⌋
        operacaoPendente = 4;
        fpu->z = floor_custom(fpu->z_float);
        break;
    case 0b01001:
        //  Arredondamento ∥Z ∥
        operacaoPendente = 4;
        fpu->z = round_custom(fpu->z_float);
        break;
    default:
        fpu->OP_ST |= 0x20;
        operacaoPendente = 2;
        break;
    }
}

void watchdogCounter(Watchdogs *watchdog)
{

    if (watchdog->en)
    {
        watchdog->counter--;
    }
    else
    {
        watchdog->watchdogState = 0;
    }

    if (watchdog->counter == 0)
    {
        watchdog->watchdogState = 0;
        pendente = 1;
        operacaoPendente = 1;
    }
}

char *strRegistrador(int num, int type, int isSbrCbr)
{
    char *result = (char *)malloc(10 * sizeof(char));
    if (result == NULL)
    {
        return NULL;
    }

    if (isSbrCbr)
    {
        snprintf(result, 10, "%c[%d]", type ? 'r' : 'R', num);
        return result;
    }

    if (num < 26 || num > 31)
    {

        snprintf(result, 10, "%c%d", type ? 'r' : 'R', num);
        return result;
    }

    switch (num)
    {
    case 26:
        strcpy(result, type ? "cr" : "CR");
        break;
    case 27:
        strcpy(result, type ? "ipc" : "IPC");
        break;
    case 28:
        strcpy(result, type ? "ir" : "IR");
        break;
    case 29:
        strcpy(result, type ? "pc" : "PC");
        break;
    case 30:
        strcpy(result, type ? "sp" : "SP");
        break;
    case 31:
        strcpy(result, type ? "sr" : "SR");
        break;
    default:
        snprintf(result, 10, "%c%d", type ? 'r' : 'R', num);
    }

    return result;
}
int calcQtdRegistradores(int v, int w, int x, int y, int z)
{
    int resultado = 1;
    int aux = v;
    int index = 1;

    if (aux != 0)
    {
        while (aux != 0)
        {
            switch (index)
            {
            case 1:
                aux = w;
                break;
            case 2:
                aux = x;
                break;
            case 3:
                aux = y;
                break;
            default:
                aux = z;
                break;
            }
            if (aux != 0)
            {

                resultado += 1;
            }

            if (index == 4)
            {
                return resultado;
            }
            index++;
        }

        return resultado;
    }
    else
    {
        return 0;
    }
}

void initFPU(FPU *fpu)
{
    fpu->z_float = 0;
    fpu->y_float = 0;
    fpu->x_float = 0;
    fpu->OP_ST = 0;
    fpu->x = 0;
    fpu->y = 0;
    fpu->z = 0;
}

void initWatchdogs(Watchdogs *watchdog)
{
    watchdog->en = 0;
    watchdog->counter = 0;
    watchdog->watchdogState = 0;
}

void initTerminal(Terminal *terminal)
{

    terminal->buffer = (char *)malloc((32 * 1024) * sizeof(char));
    terminal->size = 0;
}

int main(int argc, char *argv[])
{
    // FILE *input = fopen(argv[1], "r");
    // FILE *output = fopen(argv[2], "w");
    FILE *input = fopen("input.txt", "r");
    FILE *output = fopen("output.txt", "w");
    if (input == NULL)
    {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    uint32_t R[32] = {0};
    uint32_t *MEM32 = (uint32_t *)calloc(32 * 1024, sizeof(uint32_t));
    Terminal *terminal = malloc(sizeof(Terminal));
    initTerminal(terminal);
    Watchdogs *watchdog = malloc(sizeof(Watchdogs));
    initWatchdogs(watchdog);
    FPU *fpu = malloc(sizeof(FPU));
    initFPU(fpu);

    if (MEM32 == NULL)
    {
        perror("Erro ao alocar MEM32");
        fclose(input);
        return 1;
    }

    int a = 0;

    while (fscanf(input, "%x", &MEM32[a]) == 1)
    {
        a++;
    }

    fprintf(output, "[START OF SIMULATION]\n");

    uint8_t executa = 1;

    while (executa)
    {
        char instrucao[30] = {0};
        uint8_t z = 0, x = 0, y = 0, i = 0, l = 0;
        uint32_t pc = R[29], xyl = 0, auxUnsigned32 = 0;
        int32_t auxSigned32 = 0, aux_mask = 0;
        int64_t auxSigned64 = 0;
        uint64_t auxUnsigned64 = 0;

        R[28] = MEM32[R[29] >> 2];

        uint8_t opcode = (R[28] & (0b111111 << 26)) >> 26;

        if (watchdog->watchdogState)
        {
            if (watchdog->counter != 0)
            {
                watchdogCounter(watchdog);
            }
        }

        switch (opcode)
        {
        // mov
        case 0b000000:
            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            R[z] = xyl;

            sprintf(instrucao, "mov %s,%u", strRegistrador(z, 1, 0), xyl);
            fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), xyl);
            break;

        // divI
        case 0b010101:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_divi = R[28] & 0xFFFF;
            auxSigned32 = (int32_t)aux_divi;

            if (auxSigned32 != 0)
            {
                R[z] = (int32_t)R[x] / auxSigned32;

                if (R[z] == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }

                R[31] &= ~ZD;
            }
            else
            {
                if (R[31] & 0x02)
                {

                    R[31] |= ZD;
                    interruptionState = 1;
                    causeOfInterruption = 3;
                }
            }

            sprintf(instrucao, "divi %s,%s,%d", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), auxSigned32);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), auxSigned32, R[z], R[31]);
            break;

        // movs
        case 0b000001:

            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            auxSigned32 = (R[28] >> 20) & 1;
            aux_mask = auxSigned32 ? 0xFFE00000 : 0x00000000;
            auxSigned32 = aux_mask | xyl;

            R[z] = auxSigned32;

            sprintf(instrucao, "movs %s,%d", strRegistrador(z, 1, 0), auxSigned32);

            fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), R[z]);
            break;

        // add
        case 0b000010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            uint64_t aux_add = (uint64_t)R[x] + (uint64_t)R[y];
            R[z] = R[x] + R[y];

            if (!R[z])
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if ((aux_add & (1ULL << 32)))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] & R[y]) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "add %s,%s,%s", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(y, 0, 0), R[z], R[31]);
            break;

        // addI
        case 0b010010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_addI = R[28] & 0xFFFF;

            auxSigned64 = (int64_t)R[x] + (int64_t)aux_addI;
            R[z] = R[x] + (int32_t)aux_addI;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if (auxSigned64 & (1ULL << 32))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] == i) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "addi %s,%s,%d", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), (int32_t)aux_addI);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s+0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), (int32_t)aux_addI, R[z], R[31]);

            break;

        // sub
        case 0b000011:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] - R[y];

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if (((uint64_t)R[z] & (1ULL << 31)))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] ^ R[y]) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "sub %s,%s,%s", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s-%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(y, 0, 0), R[z], R[31]);

            break;

        // subI
        case 0b010011:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_i = R[28] & 0xFFFF;

            auxSigned64 = (int32_t)R[x] - (int32_t)aux_i;
            R[z] = R[x] - aux_i;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if (auxSigned64 & (1ULL << 32))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] ^ i) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "subi %s,%s,%d", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), (int32_t)aux_i);

            fprintf(output, "0x%08X:\t%-25s\t%s=%s-0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), (int32_t)aux_i, R[z], R[31]);
            break;

        // caso variado
        case 0b000100:
            switch ((R[28] >> 8) & 0x7)
            {
            // mul
            case 0b000:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                auxUnsigned64 = ((uint64_t)R[x]) * ((uint64_t)R[y]);

                R[z] = (uint32_t)auxUnsigned64;
                R[l] = (uint32_t)(auxUnsigned64 >> 32);

                auxUnsigned64 = (uint64_t)R[l] << 32 | R[z];

                if (auxUnsigned64 == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }
                if (R[l] != 0)
                {
                    R[31] |= CY;
                }
                else
                {
                    R[31] &= ~CY;
                }

                sprintf(instrucao, "mul %s,%s,%s,%s", strRegistrador(l, 1, 0), strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
                fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%016" PRIX64 ",SR=0x%08X\n", R[29], instrucao, strRegistrador(l, 0, 0), strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(y, 0, 0), auxUnsigned64, R[31]);
                break;

            // sll
            case 0b001:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                auxUnsigned64 = ((uint64_t)R[z] << 32) | R[x];

                if (auxUnsigned64 == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }
                auxUnsigned64 = (((int64_t)R[z] << 32) | (int64_t)R[y]) << (l + 1);

                if (R[z] != 0)
                {
                    R[31] |= CY;
                }
                else
                {
                    R[31] &= ~CY;
                }

                R[z] = (auxUnsigned64 >> 32) & 0xFFFFFFFF;
                R[y] = auxUnsigned64 & 0xFFFFFFFF;

                sprintf(instrucao, "sll %s,%s,%s,%u", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(x, 1, 0), l);
                fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%016" PRIX64 ",SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), l + 1, auxUnsigned64, R[31]);
                break;

            // muls
            case 0b010:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                int64_t xy = (int32_t)R[x] * (int32_t)R[y];
                R[z] = xy;
                R[l] = (xy >> 32);

                int64_t aux_muls = ((int64_t)R[l] << 32) | R[z];

                if (aux_muls == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }

                if (R[l] != 0)
                {
                    R[31] |= OV;
                }
                else
                {
                    R[31] &= ~OV;
                }
                sprintf(instrucao, "muls r%u,r%u,r%u,r%u", l, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%016" PRIX64 ",SR=0x%08X\n", R[29], instrucao, l, z, x, y, aux_muls, R[31]);
                break;

            // sla
            case 0b011:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                auxSigned64 = ((int32_t)R[z]) | (int32_t)R[x];

                auxSigned64 = (((int32_t)R[z]) | (uint32_t)R[y]) << (l + 1);
                if (auxSigned64 == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }

                if (R[z] != 0)
                {
                    R[31] |= OV;
                }
                else
                {
                    R[31] &= ~OV;
                }

                R[x] = auxSigned64;

                sprintf(instrucao, "sla r%u,r%u,r%u,%d", z, x, x, l);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u<<%d=0x%016X,SR=0x%08X\n", R[29], instrucao, z, x, z, x, l + 1, R[x], R[31]);
                break;

            // div
            case 0b100:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                if (R[y] != 0)
                {

                    R[l] = R[x] % R[y],
                    R[z] = R[x] / R[y];

                    if (R[z] == 0)
                    {
                        R[31] |= ZN;
                    }
                    else
                    {
                        R[31] &= ~ZN;
                    }

                    if (R[l] != 0)
                    {
                        R[31] |= CY;
                    }
                    else
                    {
                        R[31] &= ~CY;
                    }
                }

                if (R[y] == 0)
                {
                    R[31] |= ZD;
                }
                else
                {
                    R[31] &= ~ZD;
                }

                // if (R[IE])
                // {
                //     interruptionState = 1;
                //     causeOfInterruption = 3;
                // }
                sprintf(instrucao, "div r%u,r%u,r%u,r%u", l, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u=R%u%%R%u=0x%08X,R%u=R%u/R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, l, x, y, R[l], z, x, y, R[z], R[31]);
                break;

            // srl
            case 0b101:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                auxUnsigned64 = ((uint64_t)R[z] << 32) | R[x];

                if (auxUnsigned64 == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }
                auxUnsigned64 = (((uint64_t)R[z] << 32) | (uint64_t)R[y]) >> (l + 1);

                if (R[z] != 0)
                {
                    R[31] |= CY;
                }
                else
                {
                    R[31] &= ~CY;
                }

                R[x] = auxUnsigned64;
                sprintf(instrucao, "slr r%u,r%u,r%u,%u", z, x, x, l);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u>>%u=0x%016X,SR=0x%08X\n", R[29], instrucao, z, x, z, x, l + 1, R[x], R[31]);
                break;

            // divs
            case 0b110:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                if (R[y] == 0)
                {
                    if (R[y] == 0)
                    {
                        R[31] |= ZD;
                    }
                    else
                    {
                        R[31] &= ~ZD;
                    }

                    R[l] = 0;
                    R[z] = 0;
                }
                else
                {
                    R[l] = (int32_t)R[x] % (int32_t)R[y],
                    R[z] = (int32_t)R[x] / (int32_t)R[y];
                }

                if (R[z] == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }

                if (R[l] != 0)
                {
                    R[31] |= CY;
                }
                else
                {
                    R[31] &= ~CY;
                }

                sprintf(instrucao, "divs r%u,r%u,r%u,r%u", l, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u=R%u%%R%u=0x%08X,R%u=R%u/R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, l, x, y, (int32_t)R[l], z, x, y, (int32_t)R[z], R[31]);
                break;

            // sra
            case 0b111:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                auxSigned64 = ((int64_t)R[z] << 32) | R[x];

                if (auxSigned64 == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }

                auxSigned64 = (((int64_t)R[z] << 32) | R[y]) >> (l + 1);

                if (R[z] != 0)
                {
                    R[31] |= OV;
                }
                else
                {
                    R[31] &= ~OV;
                }

                R[x] = auxSigned64;

                sprintf(instrucao, "sra r%u,r%u,r%u,%u", z, x, x, l);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u>>%u=0x%016" PRIX64 ",SR=0x%08X\n", R[29], instrucao, z, x, z, x, l + 1, auxSigned64, R[31]);
                break;
            default:
                fprintf(output, "Instrucao desconhecida!\n");

                // Parar a execucao
                executa = 0;
            }

            break;

        // mulI
        case 0b010100:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_muli = (R[28] & 0xFFFF);

            int32_t aux_mulI32 = (int32_t)aux_muli;
            R[z] = R[x] * aux_mulI32;
            auxSigned64 = (int32_t)R[x] * aux_mulI32;
            auxSigned32 = auxSigned64 >> 32;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }
            if (auxSigned32 != 0)
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "muli r%u,r%u,%d", z, x, aux_mulI32);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u*0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, aux_mulI32, R[z], R[31]);

            break;

        // modI
        case 0b010110:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_modi = R[28] & 0xFFFF;
            auxSigned32 = (int32_t)aux_modi;

            R[z] = (int32_t)R[x] % auxSigned32;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if (aux_modi == 0)
            {
                R[31] |= ZD;
            }
            else
            {
                R[31] &= ~ZD;
            }

            sprintf(instrucao, "modi %s,%s,%d", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), aux_modi);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), aux_modi, R[z], R[31]);

            break;

        // cmp
        case 0b000101:
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            auxUnsigned32 = R[x] - R[y];
            auxUnsigned64 = (uint64_t)R[x] - (uint64_t)R[y];
            if (auxUnsigned32 == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((auxUnsigned32 & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if (auxUnsigned64 & (1ULL << 32))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] ^ R[y]) & (1 << 31)) && ((R[x] ^ auxUnsigned32) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "cmp %s,%s", strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instrucao, R[31]);

            break;

        // cmpI
        case 0b010111:
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t imediato = R[28] & 0xFFFF;

            int32_t aux_cmpi = (int32_t)imediato;
            auxSigned64 = (int64_t)R[x] - (int64_t)aux_cmpi;

            if (auxSigned64 == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((auxSigned64 & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if (auxSigned64 & (1ULL << 32))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] ^ aux_cmpi) & (1 << 31)) && ((auxSigned64 ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "cmpi %s,%d", strRegistrador(x, 1, 0), aux_cmpi);
            fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instrucao, R[31]);

            break;

        // and
        case 0b000110:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] & R[y];

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            sprintf(instrucao, "and %s,%s,%s", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(y, 0, 0), R[z], R[31]);

            break;

        // or
        case 0b000111:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] | R[y];

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            sprintf(instrucao, "or %s,%s,%s", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(y, 0, 0), R[z], R[31]);

            break;

        // not
        case 0b001000:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;

            R[z] = ~R[x];

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            sprintf(instrucao, "not %s,%s", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), R[z], R[31]);

            break;

        // xor
        case 0b001001:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] ^ R[y];

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            sprintf(instrucao, "xor %s,%s,%s", strRegistrador(z, 1, 0), strRegistrador(x, 1, 0), strRegistrador(y, 1, 0));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0, 0), strRegistrador(x, 0, 0), strRegistrador(y, 0, 0), R[z], R[31]);

            break;

        // l8
        case 0b011000:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_l8 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_l8;

            switch (R[x] + auxUnsigned32)
            {
            case COUNTER:
                R[z] = watchdog->counter;
                break;
            case st:
                R[z] = fpu->OP_ST & 0x3F;
                break;
            case op:
                R[z] = fpu->OP_ST & 0x1F;
                break;
            case X:
            case x_:
                R[z] = fpu->x;
                break;
            case Z:
            case z_:
                R[z] = fpu->z;
                break;
            case Y:
            case y_:
                R[z] = fpu->y;
                break;
            case OUT:
                terminal->buffer[terminal->size] = R[x] + i;
                terminal->size += 1;
                break;
            case IN:
                R[z] = terminal->buffer[terminal->size];
                terminal->size += 1;
                break;
            default:
                if (R[x] + i > (32 * 1024))
                {
                    executa = 0;
                    break;
                }
                else
                {
                    R[z] = (((uint8_t *)(&MEM32[(R[x] + auxUnsigned32) >> 2]))[3 - ((R[x] + auxUnsigned32) % 4)]);
                }
                break;
            }
            sprintf(instrucao, "l8 r%u,[r%u%s%i]", z, x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + auxUnsigned32, R[z]);

            break;

        // l16
        case 0b011001:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_l16 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_l16;

            switch (R[x] + auxUnsigned32)
            {
            case COUNTER:
                R[z] = watchdog->counter;
                break;
            case st:
                R[z] = fpu->OP_ST & 0x3F;
                break;
            case op:
                R[z] = fpu->OP_ST & 0x1F;
                break;
            case X:
            case x_:
                R[z] = fpu->x;
                break;
            case Z:
            case z_:
                R[z] = fpu->z;
                break;
            case Y:
            case y_:
                R[z] = fpu->y;
                break;
            case OUT:
                terminal->buffer[terminal->size] = R[x] + i;
                terminal->size += 1;
                break;
            default:
                if (R[x] + i > (32 * 1024))
                {
                    executa = 0;
                    break;
                }
                else
                {
                    R[z] = ((uint16_t *)(&MEM32[(R[x] + auxUnsigned32) >> 1]))[1 - ((R[x] + auxUnsigned32) % 2)];
                }
                break;
            }

            sprintf(instrucao, "l16 r%u,[r%u%s%i]", z, x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%04X\n", R[29], instrucao, z, (R[x] + auxUnsigned32) << 1, R[z]);

            break;

        // l32
        case 0b011010:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;
            uint32_t auxL32 = (R[x] + i) << 2;
            uint32_t addFinal = 0;

            switch (auxL32)
            {
            case COUNTER:
                R[z] = watchdog->counter;
                addFinal = COUNTER;
                break;
            case st:
                R[z] = fpu->OP_ST & 0x3F;
                addFinal = st;
                if (fpu->OP_ST & 0x1F)
                {
                    ciclo = calc_ciclos(fpu);
                    calcFPU(fpu);
                    if (operacaoPendente == 3)
                        ciclo += 1;
                }
                break;
            case op:
                R[z] = fpu->OP_ST & 0x1F;
                addFinal = op;
                break;
            case X:
                addFinal = X;
                R[z] = fpu->x;
                break;
            case x_:
                R[z] = fpu->x;
                addFinal = x_;
                break;
            case Z:
                R[z] = fpu->z;
                addFinal = Z;
                break;
            case z_:
                R[z] = fpu->z;
                addFinal = z_;
                break;
            case Y:
                R[z] = fpu->y;
                addFinal = Y;
                break;
            case y_:
                R[z] = fpu->y;
                addFinal = y_;
                break;
            case OUT:
                terminal->buffer[terminal->size] = R[x] + i;
                terminal->size += 1;
                addFinal = OUT;
                break;
            case IN:
                R[z] = terminal->buffer[terminal->size];
                terminal->size += 1;
                addFinal = IN;
                break;
            default:
                if (R[x] + i > (32 * 1024))
                {
                    executa = 0;
                    break;
                }
                else
                {
                    R[z] = MEM32[(R[x] + i)];
                    addFinal = auxL32;
                }
                break;
            }
            sprintf(instrucao, "l32 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%08X\n", R[29], instrucao, z, addFinal, R[z]);

            break;

        // s8
        case 0b011011:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_s8 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_s8;

            switch (R[x] + auxUnsigned32)
            {
            case OUT:
                terminal->buffer[terminal->size] = R[z] & 0xFF;
                terminal->size++;
                break;
            case IN:
                terminal->buffer[terminal->size] = R[z] & 0xFF;
                terminal->size++;
                break;
            case st:
                fpu->OP_ST = R[z] & 0x20;
                break;
            case op:
                fpu->OP_ST = R[z] & 0x1F;
                break;
            case X:
                fpu->x = R[z];
                break;
            case Y:
                fpu->y = R[z];
                break;
            case Z:
                fpu->z = R[z];
                break;
            case COUNTER:
                watchdog->counter = R[z] & 0x7FFFFFFF;
                watchdog->watchdogState = 1;
                watchdog->en = (R[z] >> 31) & 0x01;
                break;
            default:
                (((uint8_t *)(&MEM32[(R[x] + auxUnsigned32) >> 2]))[3 - ((R[x] + auxUnsigned32) % 4)]) = R[z];
                break;
            }

            sprintf(instrucao, "s8 [r%u%s%i],r%u", x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32, z);
            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%02X\n", R[29], instrucao, R[x] + auxUnsigned32, z, R[z]);

            break;

        // s16
        case 0b011100:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_s16 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_s16;

            switch (R[x] + auxUnsigned32)
            {
            case OUT:
                terminal->buffer[terminal->size] = R[z] & 0xFF;
                terminal->size++;
                break;
            case st:
                fpu->OP_ST = R[z] & 0x20;
                break;
            case op:
                fpu->OP_ST = R[z] & 0x1F;
                break;
            case X:
                fpu->x = R[z];
                break;
            case Y:
                fpu->y = R[z];
                break;
            case Z:
                fpu->z = R[z];
                break;
            case COUNTER:
                watchdog->counter = R[z] & 0x7FFFFFFF;
                watchdog->watchdogState = 1;
                watchdog->en = (R[z] >> 31) & 0x01;
                break;
            default:
                ((uint16_t *)(&MEM32[(R[x] + auxUnsigned32) >> 1]))[1 - ((R[x] + auxUnsigned32) % 2)] = R[z];
                break;
            }
            sprintf(instrucao, "s16 [r%u%s%i],r%u", x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32, z);
            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%04X\n", R[29], instrucao, (R[x] + auxUnsigned32) << 1, z, R[z]);

            break;

        // s32
        case 0b011101:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;
            uint32_t mapeamento = (R[x] + i) << 2;

            switch (mapeamento)
            {
            case COUNTER:
                watchdog->counter = R[z] & 0x7FFFFFFF;
                watchdog->en = (R[z] >> 31) & 0x01;
                watchdog->watchdogState = 1;
                break;
            case X:
                fpu->x = R[z];
                break;
            case Y:
                fpu->y = R[z];
                break;
            case Z:
                fpu->z = R[z];
                break;
            case st:
                fpu->OP_ST = R[z] & 0x3F;
                break;
            case op:
                fpu->OP_ST = R[z] & 0x1F;
                break;
            default:
                if (R[x] + i > (32 * 1024))
                {
                    executa = 0;
                    break;
                }
                MEM32[R[x] + i] = R[z];
                break;
            }

            sprintf(instrucao, "s32 [r%u%s%i],r%u", x, (i >= 0) ? ("+") : (""), i, z);
            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%08X\n", R[29], instrucao, (R[x] + i) << 2, z, R[z]);

            break;

        // bae
        case 0b101010:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bae = aux_mask | (R[28] & 0x3FFFFFF);

            if (!((R[31] >> CY) & 1))
            {
                R[29] = R[29] + (aux_bae << 2);
            }
            sprintf(instrucao, "bae %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bat
        case 0b101011:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bat = aux_mask | (R[28] & 0x3FFFFFF);

            if (!((R[31] >> (ZN & 31)) & 1) && !((R[31] >> CY) & 1))
            {
                R[29] = R[29] - 4 + (aux_bat << 2);
            }

            sprintf(instrucao, "bat %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bbe
        case 0b101100:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bbe = aux_mask | (R[28] & 0x3FFFFFF);

            if ((R[31] >> (ZN & 32)) || ((R[31] >> CY) & 1))
            {
                R[29] = R[29] + (aux_bbe << 2);
            }

            sprintf(instrucao, "bbe %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bbt
        case 0b101101:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bbt = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> CY) & 1))
            {
                R[29] = R[29] + (aux_bbt << 2);
            }

            sprintf(instrucao, "bbt %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // beq
        case 0b101110:
            auxSigned32 = (R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 6))
            {
                R[29] = R[29] + (auxSigned32 << 2);
            }

            sprintf(instrucao, "beq %i  ", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bge
        case 0b101111:
            auxSigned32 = (R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 3) == (R[31] & 1 << 4))
            {
                R[29] = R[29] + (auxSigned32 << 2);
                sprintf(instrucao, "bge %i ", R[28] & 0x3FFFFFF);
                fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
            }
            else
            {
                sprintf(instrucao, "bge %i ", R[28] & 0x3FFFFFF);
                fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
            }

            break;

        // bgt
        case 0b110000:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            int32_t aux_bgt = aux_mask | (R[28] & 0x3FFFFFF);

            if ((((R[31] >> (ZN & 31))) == 0) && (((R[31] >> SN) & 1) == ((R[31] >> OV) & 1)))
            {
                R[29] = R[29] + (aux_bgt << 2);
            }

            sprintf(instrucao, "bgt %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
            break;

        // biv
        case 0b110001:
            auxSigned32 = (R[28] & 0x3FFFFFF);
            sprintf(instrucao, "biv %i", R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 2))
            {
                R[29] = R[29] + (auxSigned32 << 2);
            }

            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
            break;

        // ble
        case 0b110010:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_ble = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> (ZN & 31))) || (((R[31] >> SN) & 1) != ((R[31] >> OV) & 1)))
            {
                R[29] = R[29] + (aux_ble << 2);
            }

            sprintf(instrucao, "ble %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
            break;

        // blt
        case 0b110011:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxSigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_blt = aux_mask | (R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 3) != (R[31] & 1 << 4))
            {
                R[29] = R[29] + (aux_blt << 2);
            }

            sprintf(instrucao, "blt %d", aux_blt);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bne
        case 0b110100:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxSigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bne = aux_mask | (R[28] & 0x3FFFFFF);

            if (!(R[31] & 1 << 6))
            {
                R[29] = R[29] + (aux_bne << 2);
            }

            sprintf(instrucao, "bne %d", aux_bne);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bni
        case 0b110101:
            auxSigned32 = (R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 2) == 0)
            {
                R[29] = R[29] + (auxSigned32 << 2);
            }

            sprintf(instrucao, "bni %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bnz
        case 0b110110:
            pc = R[29];
            auxSigned32 = (R[28] & 0x3FFFFFF);

            sprintf(instrucao, "bnz %i", R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 5) == 0)
            {
                R[29] = R[29] + (auxSigned32 << 2);
            }

            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bun
        case 0b110111:
            pc = R[29];

            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t auxBun = aux_mask | (R[28] & 0x3FFFFFF);

            R[29] += (uint32_t)(auxBun << 2);

            sprintf(instrucao, "bun %i", auxBun);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bzd
        case 0b111000:
            pc = R[29];
            uint32_t aux_bzd = (R[28] & 0x3FFFFFF);

            sprintf(instrucao, "bzd %i", R[28] & 0x3FFFFFF);
            if ((R[31] & 1 << 5))
            {
                R[29] = R[29] + (aux_bzd << 2);
            }

            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // int
        case 0b111111:

            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            auxUnsigned32 = aux_mask | (R[28] & 0x3FFFFFF);

            if (auxUnsigned32 == 0)
            {
                executa = 0;
                interruptionState = 0;
                sprintf(instrucao, "int 0");
                fprintf(output, "0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);
            }
            else
            {
                sprintf(instrucao, "int %d", auxUnsigned32);
                identifiesInterruption(4, auxUnsigned32, R, output, instrucao, MEM32);
            }

            break;

            break;

        // ret
        case 0b011111:
            R[SP] += 4;
            auxUnsigned32 = R[PC];
            R[PC] = MEM32[R[SP]];
            R[PC] -= 4;

            sprintf(instrucao, "ret");
            fprintf(output, "0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%08X\n", auxUnsigned32, instrucao, R[SP], MEM32[R[SP]]);
            break;

        case 0b100001:

            if (R[28] & 0x1)
            {
                // sbr
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;

                R[z] |= (0b1 << x);
                sprintf(instrucao, "sbr %s[%d]", strRegistrador(z, 1, 0), x);
                fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[PC], instrucao, strRegistrador(z, 0, 0), R[z]);
            }
            else
            {
                // cbr
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;

                R[z] &= ~(0b1 << x);

                sprintf(instrucao, "cbr %s[%d]", strRegistrador(z, 1, 0), x);
                fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[PC], instrucao, strRegistrador(z, 0, 0), R[z]);
            }

            break;

        // push
        case 0b001010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;
            uint8_t v = (R[28] & (0b11111 << 6)) >> 6;
            uint8_t w = (R[28] & 0b11111);

            int qtdRegistradores = calcQtdRegistradores(v, w, x, y, z);

            if (qtdRegistradores > 0)
            {
                uint8_t regsPush[5] = {v, w, x, y, z};

                char *formatosPush[] = {
                    "",
                    "push %s",
                    "push %s,%s",
                    "push %s,%s,%s",
                    "push %s,%s,%s,%s",
                    "push %s,%s,%s,%s,%s"};

                sprintf(instrucao, formatosPush[qtdRegistradores], strRegistrador(regsPush[0], 1, 0), strRegistrador(regsPush[1], 1, 0), strRegistrador(regsPush[2], 1, 0), strRegistrador(regsPush[3], 1, 0), strRegistrador(regsPush[4], 1, 0));
                fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{", R[PC], instrucao, R[SP]);

                for (int i = 0; i < qtdRegistradores; i++)
                {

                    MEM32[R[SP]] = R[regsPush[i]];
                    fprintf(output, "0x%08X", MEM32[R[SP]]);
                    R[SP] -= 4;
                    if (i + 1 < qtdRegistradores)
                    {
                        fprintf(output, ",");
                    }
                }

                fprintf(output, "}={");

                for (int i = 0; i < qtdRegistradores; i++)
                {
                    fprintf(output, "%s", strRegistrador(regsPush[i], 0, 0));
                    if (i + 1 < qtdRegistradores)
                    {
                        fprintf(output, ",");
                    }
                }
                fprintf(output, "}\n");
            }
            else
            {
                sprintf(instrucao, "push -");
                fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{}={}\n", R[PC], instrucao, R[SP]);
            }
            break;

            // pop
        case 0b001011:
            // Extração dos valores dos registradores a partir do código de operação
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;
            v = (R[28] & (0b11111 << 6)) >> 6;
            w = (R[28] & 0b11111);

            // Calcula a quantidade de registradores envolvidos na operação
            int qtdRegistradores_pop = calcQtdRegistradores(v, w, x, y, z);

            if (qtdRegistradores_pop > 0)
            {
                uint8_t regsPop[5] = {v, w, x, y, z};

                char *formatosPop[] = {
                    "",
                    "pop %s",
                    "pop %s,%s",
                    "pop %s,%s,%s",
                    "pop %s,%s,%s,%s",
                    "pop %s,%s,%s,%s,%s"};

                sprintf(instrucao, formatosPop[qtdRegistradores_pop], strRegistrador(regsPop[0], 1, 0), strRegistrador(regsPop[1], 1, 0), strRegistrador(regsPop[2], 1, 0), strRegistrador(regsPop[3], 1, 0), strRegistrador(regsPop[4], 1, 0));
                fprintf(output, "0x%08X:\t%-25s\t{", R[PC], instrucao);

                for (int i = 0; i < qtdRegistradores_pop; i++)
                {
                    fprintf(output, "%s", strRegistrador(regsPop[i], 0, 0));
                    if (i + 1 < qtdRegistradores_pop)
                    {
                        fprintf(output, ",");
                    }
                }

                fprintf(output, "}=MEM[0x%08X]{", R[SP]);

                for (int i = 0; i < qtdRegistradores_pop; i++)
                {
                    R[SP] += 4;
                    R[regsPop[i]] = MEM32[R[SP]];
                    fprintf(output, "0x%08X", MEM32[R[SP]]);
                    if (i + 1 < qtdRegistradores_pop)
                    {
                        fprintf(output, ",");
                    }
                }
                fprintf(output, "}\n");
            }
            else
            {
                sprintf(instrucao, "pop -");
                fprintf(output, "0x%08X:\t%-25s\t{}=MEM[0x%08X]{}\n", R[PC], instrucao, R[SP]);
            }
            // return 0;
            break;

            // reti
        case 0b100000:
            uint32_t pcAnterior = R[PC];

            R[SP] += 4;
            uint32_t ipcAtual = R[SP];
            R[IPC] = MEM32[ipcAtual];

            R[SP] += 4;
            uint32_t crAtual = R[SP];
            R[CR] = MEM32[crAtual];

            R[SP] += 4;
            uint32_t pcAtual = R[SP];
            R[PC] = MEM32[pcAtual];

            sprintf(instrucao, "reti");
            fprintf(output, "0x%08X:\t%-25s\tIPC=MEM[0x%08X]=0x%08X,CR=MEM[0x%08X]=0x%08X,PC=MEM[0x%08X]=0x%08X\n", pcAnterior, instrucao, ipcAtual, R[IPC], crAtual, R[CR], pcAtual, R[PC]);
            R[PC] -= 4;
            break;

        // call tipo S
        case 0b111001:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_callS = aux_mask | (R[28] & 0x3FFFFFF);

            MEM32[R[SP]] = R[PC] + 4;
            uint32_t aux_pc = R[PC];
            R[PC] += (aux_callS << 2);

            sprintf(instrucao, "call %d", aux_callS);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", aux_pc, instrucao, R[PC] + 4, R[SP], MEM32[R[SP]]);

            R[SP] -= 4;
            break;

        // call tipo F
        case 0b011110:
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_callF = R[28] & 0xFFFF;
            auxUnsigned32 = R[PC];

            MEM32[R[SP]] = R[PC] + 4;
            R[PC] = (R[x] + (uint32_t)aux_callF) << 2;

            sprintf(instrucao, "call [r%d+%d]", x, (uint32_t)aux_callF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", auxUnsigned32, instrucao, R[PC], R[SP], MEM32[R[SP]]);
            R[PC] -= 4;
            R[SP] -= 4;
            break;

        // Instrucao desconhecida
        default:
            fprintf(output, "[INVALID INSTRUCTION @ 0x%08X]\n", R[29]);
            identifiesInterruption(2, 0, R, output, instrucao, MEM32);
        }

        if (ciclo != 0)
        {
            ciclo--;

            if (!ciclo)
                pendente = 1;
        }

        if (R[31] & 0x02)
        {

            if (pendente)
            {
                switch (operacaoPendente)
                {
                case 1:
                    identifiesInterruption(5, 0, R, output, instrucao, MEM32);
                    fpu->OP_ST &= ~0x1F;
                    break;
                case 2:
                    identifiesInterruption(6, 0, R, output, instrucao, MEM32);
                    fpu->OP_ST &= ~0x1F;
                    break;
                case 3:
                    identifiesInterruption(7, 0, R, output, instrucao, MEM32);
                    fpu->OP_ST &= ~0x1F;
                    break;
                case 4:
                    identifiesInterruption(8, 0, R, output, instrucao, MEM32);
                    fpu->OP_ST &= ~0x1F;
                    break;
                default:
                    break;
                }
                pendente = 0;
            }

            if (interruptionState)
            {

                identifiesInterruption(causeOfInterruption, 0, R, output, instrucao, MEM32);
            }
        }

        R[29] = R[29] + 4;
    }

    if (terminal->size > 0)
    {
        fprintf(output, "[TERMINAL]\n");
        fprintf(output, "%s\n", terminal->buffer);
    }

    fprintf(output, "[END OF SIMULATION]\n");

    fclose(input);
    fclose(output);
    free(MEM32);
    free(terminal);
    free(fpu);
    free(watchdog);
    return 0;
}