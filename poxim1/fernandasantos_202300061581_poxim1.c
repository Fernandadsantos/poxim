#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define ZN (1 << 6)
#define CY (1 << 0)
#define OV (1 << 3)
#define SN (1 << 4)
#define ZD (1 << 5)
#define IC (1 << 2)

// R[31] |= CY;    // Define o bit como 1
// R[31] &= ~CY;     // Define o bit como 0

int main()
{
    FILE *input = fopen("1_erro.hex", "r");
    if (input == NULL)
    {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    uint32_t R[32] = {0};
    uint32_t *MEM32 = (uint32_t *)calloc(32 * 1024, sizeof(uint32_t));

    if (MEM32 == NULL)
    {
        perror("Erro ao alocar MEM32");
        fclose(input);
        return 1;
    }

    int a = 0;

    while (fscanf(input, "%x", &MEM32[a]) == 1)
        a++;

    printf("[START OF SIMULATION]\n");

    uint8_t executa = 1;

    while (executa)
    {

        char instrucao[30] = {0};
        uint8_t z = 0, x = 0, y = 0, i = 0;
        uint32_t pc = 0, xyl = 0;

        // Carregando a instrucao de 32 bits (4 bytes) da memoria indexada pelo PC (R29) no registrador IR (R28)
        R[28] = MEM32[R[29] >> 2];

        // Obtendo o codigo da operacao (6 bits mais significativos)
        uint8_t opcode = (R[28] & (0b111111 << 26)) >> 26;

        switch (opcode)
        {
        // mov
        case 0b000000:
            // Obtendo operandos
            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            // Execucao do comportamento
            R[z] = xyl;
            // Formatacao da instrucao
            sprintf(instrucao, "mov r%u,%u", z, xyl);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, xyl);
            break;

        // divI
        case 0b010101:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            if (i != 0)
            {
                R[z] = R[x] / i;
            }

            sprintf(instrucao, "divi r%u,r%u,%u", z, x, i);

            // setar as flags
            printf("0x%08X:\t%-25s\tR%u=R%u/0x%08X=0x%08X,SR=\n", R[29], instrucao, z, x, i, R[z]);
            break;

        // movs
        case 0b000001:

            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFFF;

            R[z] = (((xyl << 24) & 1) * 0b11111111111) + xyl;

            sprintf(instrucao, "movs r%u,%u", z, xyl);
            printf("0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, R[z]);
            break;

        // add
        case 0b000010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] + R[y];

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }

            if (R[z] & (1 << 32))
            {
                R[31] |= CY;
            }

            if (((R[x] ^ R[y]) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }

            sprintf(instrucao, "add r%u,r%u,r%u", z, x, y);
            printf("0x%08X:\t%-25s\tR%u=R%u+R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);
            break;

        // addI
        case 0b010010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = R[x] + i;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }

            if (R[z] & (1 << 32))
            {
                R[31] |= CY;
            }

            if (((R[x] == i) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }

            sprintf(instrucao, "addi r%u,r%u,r%u", z, x, i);
            printf("0x%08X:\t%-25s\tR%u=R%u+0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, i, R[z], R[31]);
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

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }

            if (R[z] & (1 << 32))
            {
                R[31] |= CY;
            }

            if (((R[x] ^ R[y]) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }

            sprintf(instrucao, "sub r%u,r%u,r%u", z, x, y);
            printf("0x%08X:\t%-25s\tR%u=R%u-R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);
            break;

        // subI
        case 0b010011:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;
            R[z] = R[x] - i;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }

            if ((R[z] & (1 << 31)))
            {
                R[31] |= SN;
            }

            if (R[z] & (1 << 32))
            {
                R[31] |= CY;
            }

            if (((R[x] ^ i) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }

            sprintf(instrucao, "subi r%u,r%u,r%u", z, x, i);
            printf("0x%08X:\t%-25s\tR%u=R%u-0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, i, R[z], R[31]);
            break;

        // caso variado
        case 0b000100:
            switch ((R[28] >> 8) & 0x7)
            {

            // mul
            case 0b000:
                break;

            // sll
            case 0b001:
                break;

            // muls
            case 0b010:
                break;

            // sla
            case 0b011:
                break;

            // div
            case 0b100:
                break;

            // srl
            case 0b101:
                break;

            // divs
            case 0b110:
                break;

            // sra
            case 0b111:
                break;
            default:
                // Exibindo mensagem de erro
                printf("Instrucao desconhecida!\n");
                // Parar a execucao
                executa = 0;
            }

            break;

        // mulI
        case 0b010100:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = R[x] * i;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }

            // if (((R[x] ^ i) & (1 << 31)) && ((R[z] ^ R[x]) & (1 << 31)))
            // {
            //     R[31] |= OV;
            // }

            sprintf(instrucao, "muli r%u,r%u,r%u", z, x, i);
            printf("0x%08X:\t%-25s\tR%u=R%u*0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, i, R[z], R[31]);
            break;

        // modI
        case 0b010110:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = R[x] % i;

            sprintf(instrucao, "muli r%u,r%u,r%u", z, x, i);
            printf("0x%08X:\t%-25s\tR%u=R%u\%0x%08X=0x%08X\n", R[29], instrucao, z, x, i, R[z]);
            break;

        // cmp
        case 0b000101:
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            uint32_t CMP = R[x] - R[y];

            if (CMP == 0)
            {
                R[31] |= ZN;
            }

            if ((CMP & (1 << 31)))
            {
                R[31] |= SN;
            }

            if (CMP & (1 << 32))
            {
                R[31] |= CY;
            }

            if (((R[x] ^ R[y]) & (1 << 31)) && ((R[x] ^ CMP) & (1 << 31)))
            {
                R[31] |= OV;
            }

            sprintf(instrucao, "cmp r%u,r%u", z, x);
            printf("0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instrucao, R[31]);
            break;

        // cmpI
        case 0b010111:
            break;

        // and
        case 0b000110:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] & R[y];

            sprintf(instrucao, "and r%u,r%u,r%u", z, x, y);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=R%u&R%u=0x%08X\n", R[29], instrucao, z, x, y, R[z]);
            break;

        // or
        case 0b000111:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] | R[y];

            sprintf(instrucao, "or r%u,r%u,r%u", z, x, y);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=R%u|R%u=0x%08X\n", R[29], instrucao, z, x, y, R[z]);
            break;

        // not
        case 0b001000:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;

            R[z] = ~R[x];

            sprintf(instrucao, "not r%u,r%u", z, x);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=~R%u=0x%08X\n", R[29], instrucao, z, x, y, R[z]);
            break;

        // xor
        case 0b001001:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            R[z] = R[x] ^ R[y];

            sprintf(instrucao, "xor r%u,r%u,r%u", z, x, y);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=R%u^R%u=0x%08X\n", R[29], instrucao, z, x, y, R[z]);
            break;

        // l8
        case 0b011000:
            // Otendo operandos
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = (((uint8_t *)(&MEM32[(R[x] + i) >> 2]))[3 - ((R[x] + i) % 4)]);
            // Formatacao da instrucao
            sprintf(instrucao, "l8 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + i, R[z]);
            break;

        // l16
        case 0b011001:
            break;

        // l32
        case 0b011010:
            // Otendo operandos
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;
            // Execucao do comportamento com MEM8 e MEM32
            R[z] = MEM32[R[x] + i];
            // Formatacao da instrucao
            sprintf(instrucao, "l32 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%08X\n", R[29], instrucao, z, (R[x] + i) << 2, R[z]);
            break;

        // s8
        case 0b011011:
            break;

        // s16
        case 0b011100:
            break;

        // s32
        case 0b011101:
            break;

        // bae
        case 0b101010:
            break;

        // bat
        case 0b101011:
            break;

        // bbe
        case 0b101100:
            break;

        // bbt
        case 0b101101:
            break;

        // beq
        case 0b101110:
            break;

        // bge
        case 0b101111:
            break;

        // bgt
        case 0b110000:
            break;

        // biv
        case 0b110001:
            break;

        // ble
        case 0b110010:
            break;

        // bit
        case 0b110011:
            break;

        // bne
        case 0b110100:
            break;

        // bni
        case 0b110101:
            break;

        // bnz
        case 0b110110:
            break;

        // bun
        case 0b110111:
            // Armazenando o PC antigo
            pc = R[29];
            // Execucao do comportamento
            R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);
            // Formatacao da instrucao
            sprintf(instrucao, "bun %i", R[28] & 0x3FFFFFF);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
            break;

        // bzd
        case 0b111000:
            break;

        // int
        case 0b111111:
            // Parar a execucao
            executa = 0;
            // Formatacao da instrucao
            sprintf(instrucao, "int 0");
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            printf("0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);
            break;
        // Instrucao desconhecida
        default:
            // Exibindo mensagem de erro
            printf("Instrucao desconhecida!\n");
            // Parar a execucao
            executa = 0;
        }
        R[29] = R[29] + 4;
    }

    printf("[END OF SIMULATION]\n");

    fclose(input);
    free(MEM32);
    return 0;
}