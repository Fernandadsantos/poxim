#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ZN (1 << 6)
#define CY (1 << 0)
#define OV (1 << 3)
#define SN (1 << 4)
#define ZD (1 << 5)
#define IC (1 << 2)

#define IR 28
#define PC 29
#define SP 30
#define SR 31

int calcQtdRegistradores(int v, int w, int x, int y, int z)
{
    int resultado = 0;
    int aux = v;
    int index = 1;

    if (aux != 0)
    {
        resultado += 1;
        aux = w;
        while (aux != 0)
        {
            resultado += 1;

            switch (index)
            {
            case 1:
                aux = x;
                break;
            case 2:
                aux = y;
                break;
            default:
                aux = z;
                break;
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

int main(int argc, char *argv[])
{
    FILE *input = fopen(argv[1], "r");
    FILE *output = fopen(argv[2], "w");
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
        int32_t auxSigned32 = 0;
        int64_t auxSigned64 = 0;
        uint64_t auxUnsigned64 = 0;

        // Carregando a instrucao de 32 bits (4 bytes) da memoria indexada pelo PC (R29) no registrador IR (R28)
        R[28] = MEM32[R[29] >> 2];

        // Obtendo o codigo da operacao (6 bits mais significativos)
        uint8_t opcode = (R[28] & (0b111111 << 26)) >> 26;

        switch (opcode)
        {
        // mov
        case 0b000000:
            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            R[z] = xyl;

            sprintf(instrucao, "mov r%u,%u", z, xyl);
            fprintf(output, "0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, xyl);
            break;

        // divI
        case 0b010101:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int32_t auxSigned = R[28] & 0xFFFF;

            if (auxSigned != 0)
            {
                R[z] = R[x] / auxSigned;
            }

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if (auxSigned == 0)
            {
                R[31] |= ZD;
            }
            else
            {
                R[31] &= ~ZD;
            }

            sprintf(instrucao, "divi r%u,r%u,%u", z, x, auxSigned);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u/0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, auxSigned, R[z], R[31]);
            break;

        // movs
        case 0b000001:

            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            auxSigned = (R[28] >> 20) & 1;
            int32_t aux_mask = auxSigned ? 0xFFE00000 : 0x00000000;
            auxSigned32 = aux_mask | xyl;

            R[z] = auxSigned32;

            sprintf(instrucao, "movs r%u,%d", z, auxSigned32);
            fprintf(output, "0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, R[z]);
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

            sprintf(instrucao, "add r%u,r%u,r%u", z, x, y);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u+R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);
            break;

        // addI
        case 0b010010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_addI = R[28] & 0xFFFF;

            auxSigned64 = R[x] + (int32_t)aux_addI;
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

            sprintf(instrucao, "addi r%u,r%u,%d", z, x, (int32_t)aux_addI);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u+0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, (int32_t)aux_addI, R[z], R[31]);

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

            sprintf(instrucao, "sub r%u,r%u,r%u", z, x, y);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u-R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);

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

            sprintf(instrucao, "subi r%u,r%u,%d", z, x, (int32_t)aux_i);

            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u-0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, (int32_t)aux_i, R[z], R[31]);
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

                auxUnsigned64 = ((uint64_t)R[l] & 0xFFFFFFFF00000000) | (uint64_t)R[z];
                auxUnsigned64 = R[x] * R[y];

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

                R[z] = auxUnsigned64;

                sprintf(instrucao, "mul r%u,r%u,r%u,r%u", l, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%016X,SR=0x%08X\n", R[29], instrucao, l, z, x, y, R[z], R[31]);
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

                R[x] = auxUnsigned64;
                sprintf(instrucao, "sll r%u,r%u,r%u,%u", z, x, x, l);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u<<%u=0x%016X,SR=0x%08X\n", R[29], instrucao, z, x, z, x, l + 1, R[x], R[31]);
                break;

            // muls
            case 0b010:
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;
                l = R[28] & 0b11111;

                auxSigned = ((int64_t)R[l] << 32) | (int64_t)R[z];

                if (auxSigned == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }
                auxSigned = (int32_t)R[x] * (int32_t)R[y];

                if (R[l] != 0)
                {
                    R[31] |= OV;
                }
                else
                {
                    R[31] &= ~OV;
                }
                R[z] = auxSigned;
                sprintf(instrucao, "muls r%u,r%u,r%u,r%u", l, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%016X,SR=0x%08X\n", R[29], instrucao, l, z, x, y, R[z], R[31]);
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
                    R[l] = R[x] % R[y],
                    R[z] = R[x] / R[y];
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

                sprintf(instrucao, "div r%u,r%u,r%u,%u", l, x, y, z);
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

                sprintf(instrucao, "divs r%u,r%u,r%u,%u", l, x, y, z);
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
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u>>%u=0x%016X,SR=0x%08X\n", R[29], instrucao, z, x, z, x, l + 1, R[x], R[31]);
                break;
            default:
                // Exibindo mensagem de erro
                fprintf(output, "Instrucao desconhecida!\n");

                // Parar a execucao
                executa = 0;
            }

            break;

        // mulI
        case 0b010100:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            auxSigned = (R[28] >> 15) & 1;
            aux_mask = auxSigned ? 0x1FFFF : 0x00000000;
            int32_t aux_mulI = aux_mask | i;

            R[z] = R[x] * aux_mulI;
            auxSigned64 = (int32_t)R[x] * aux_mulI;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if (auxSigned64 & (1ULL << 32))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "muli r%u,r%u,%u", z, x, i);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u*0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, i, R[z], R[31]);

            break;

        // modI
        case 0b010110:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = R[x] % i;

            if (R[z] == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if (i == 0)
            {
                R[31] |= ZD;
            }
            else
            {
                R[31] &= ~ZD;
            }

            sprintf(instrucao, "modi r%u,r%u,%u", z, x, i);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u%%0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, i, R[z], R[31]);

            break;

        // cmp
        case 0b000101:
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            uint32_t CMP = R[x] - R[y];
            auxUnsigned64 = R[x] - R[y];

            if (CMP == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((CMP & (1 << 31)))
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

            if (((R[x] ^ R[y]) & (1 << 31)) && ((R[x] ^ CMP) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "cmp r%u,r%u", z, x);
            fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instrucao, R[31]);

            break;

        // cmpI
        case 0b010111:
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t imediato = R[28] & 0xFFFF;

            int64_t CMPI = R[x] - (int32_t)imediato;

            if (CMPI == 0)
            {
                R[31] |= ZN;
            }
            else
            {
                R[31] &= ~ZN;
            }

            if ((CMPI & (1 << 31)))
            {
                R[31] |= SN;
            }
            else
            {
                R[31] &= ~SN;
            }

            if (CMPI & (1ULL << 32))
            {
                R[31] |= CY;
            }
            else
            {
                R[31] &= ~CY;
            }

            if (((R[x] ^ (int32_t)imediato) & (1 << 31)) && ((CMPI ^ R[x]) & (1 << 31)))
            {
                R[31] |= OV;
            }
            else
            {
                R[31] &= ~OV;
            }

            sprintf(instrucao, "cmpi r%u,%u", x, imediato);
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

            sprintf(instrucao, "and r%u,r%u,r%u", z, x, y);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u&R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);

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

            sprintf(instrucao, "or r%u,r%u,r%u", z, x, y);
            // Formatacao de saida em tela (deve mudar para o arquivo de saida)
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u|R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);

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

            sprintf(instrucao, "not r%u,r%u", z, x);
            fprintf(output, "0x%08X:\t%-25s\tR%u=~R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, R[z], R[31]);

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

            sprintf(instrucao, "xor r%u,r%u,r%u", z, x, y);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u^R%u=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, y, R[z], R[31]);

            break;

        // l8
        case 0b011000:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = (((uint8_t *)(&MEM32[(R[x] + i) >> 2]))[3 - ((R[x] + i) % 4)]);

            sprintf(instrucao, "l8 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + i, R[z]);

            break;

        // l16
        case 0b011001:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = ((uint16_t *)(&MEM32[(R[x] + i) >> 1]))[1 - ((R[x] + i) % 2)];

            sprintf(instrucao, "l16 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, (R[x] + i) << 1, R[z]);

            break;

        // l32
        case 0b011010:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            R[z] = MEM32[R[x] + i];

            sprintf(instrucao, "l32 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%08X\n", R[29], instrucao, z, (R[x] + i) << 2, R[z]);

            break;

        // s8
        case 0b011011:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            (((uint8_t *)(&MEM32[(R[x] + i) >> 2]))[3 - ((R[x] + i) % 4)]) = R[z];

            sprintf(instrucao, "s8 [r%u%s%i],r%u", x, (i >= 0) ? ("+") : (""), i, z);
            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%02X\n", R[29], instrucao, R[x] + i, z, R[z]);

            break;

        // s16
        case 0b011100:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            ((uint16_t *)(&MEM32[(R[x] + i) >> 1]))[1 - ((R[x] + i) % 2)] = R[z];

            sprintf(instrucao, "s16 [r%u%s%i],r%u", x, (i >= 0) ? ("+") : (""), i, z);

            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%02X\n", R[29], instrucao, (R[x] + i) << 1, z, R[z]);

            break;

        // s32
        case 0b011101:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            i = R[28] & 0xFFFF;

            MEM32[R[x] + i] = R[z];

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
                R[29] = R[29] + 4 + aux_bae;
            }
            sprintf(instrucao, "bae %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // bat
        case 0b101011:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bat = aux_mask | (R[28] & 0x3FFFFFF);

            if (!((R[31] >> ZN) & 1) && !((R[31] >> CY) & 1))
            {
                R[29] = R[29] + 4 + aux_bat;
            }
            sprintf(instrucao, "bat %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // bbe
        case 0b101100:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bbe = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> ZN) & 1) || ((R[31] >> CY) & 1))
            {
                R[29] = R[29] + 4 + aux_bbe;
            }

            sprintf(instrucao, "bbe %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // bbt
        case 0b101101:
            auxUnsigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bbt = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> CY) & 1))
            {
                R[29] = R[29] + 4 + aux_bbt;
            }

            sprintf(instrucao, "bbt %u", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // beq
        case 0b101110:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_beq = aux_mask | (R[28] & 0x3FFFFFF);

            if ((R[31] >> ZN) & 1)
            {
                R[29] = R[29] + 4 + aux_beq;
            }

            sprintf(instrucao, "beq %i  ", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X \n", pc, instrucao, R[29]);

            break;

        // bge
        case 0b101111:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bge = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> SN) & 1) == ((R[31] >> OV) & 1))
            {
                R[29] = R[29] + 4 + aux_bge;
            }

            sprintf(instrucao, "bge %i ", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // bgt
        case 0b110000:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bgt = aux_mask | (R[28] & 0x3FFFFFF);

            if ((((R[31] >> ZN) & 1) == 0) && (((R[31] >> SN) & 1) == ((R[31] >> OV) & 1)))
            {
                R[29] = R[29] + 4 + aux_bgt;
            }

            sprintf(instrucao, "bgt %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);
            break;

        // biv
        case 0b110001:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_biv = aux_mask | (R[28] & 0x3FFFFFF);

            R[29] = R[29] + 4 + aux_biv;

            sprintf(instrucao, "biv %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);
            break;

        // ble
        case 0b110010:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_ble = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> ZN) & 1) && (((R[31] >> SN) & 1) != ((R[31] >> OV) & 1)))
            {
                R[29] = R[29] + 4 + aux_ble;
            }

            sprintf(instrucao, "ble %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);
            break;

        // blt
        case 0b110011:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_blt = aux_mask | (R[28] & 0x3FFFFFF);

            if (((R[31] >> SN) & 1) != ((R[31] >> OV) & 1))
            {
                R[29] = R[29] + 4 + aux_blt;
            }

            sprintf(instrucao, "blt %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // bne
        case 0b110100:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bne = aux_mask | (R[28] & 0x3FFFFFF);

            if (!((R[31] >> ZN) & 1))
            {
                R[29] = R[29] + (aux_bne << 2);
            }

            sprintf(instrucao, "bne %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bni
        case 0b110101:
            auxSigned32 = (R[28] >> 25) & 1;
            aux_mask = auxUnsigned32 ? 0xFC000000 : 0x00000000;
            uint32_t aux_bni = aux_mask | (R[28] & 0x3FFFFFF);

            if (!((R[31] >> OV) & 1))
            {
                R[29] = R[29] + 4 + aux_bni;
            }

            sprintf(instrucao, "bni %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29]);

            break;

        // bnz
        case 0b110110:
            pc = R[29];

            if (!((R[31] >> ZD) & 1))
            {

                R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);
            }

            sprintf(instrucao, "bnz %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bun
        case 0b110111:
            pc = R[29];

            R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);

            sprintf(instrucao, "bun %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bzd
        case 0b111000:
            pc = R[29];

            R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);

            sprintf(instrucao, "bzd %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // int
        case 0b111111:
            executa = 0;
            sprintf(instrucao, "int 0");
            fprintf(output, "0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);

            break;

        // ret
        case 0b011111:
            R[SP] += 4;
            auxUnsigned32 = R[PC];
            R[PC] = MEM32[R[SP]];

            sprintf(instrucao, "ret");
            fprintf(output, "0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%08X\n", auxUnsigned32, instrucao, R[SP], MEM32[R[SP]]);
            R[PC] -= 4;
            break;

        // push
        case 0b001010:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;
            uint8_t v = (R[28] & (0b11111 << 6)) >> 6;
            uint8_t w = (R[28] & 0b11111);
            uint8_t aux_push = v;

            int qtdRegistradores = calcQtdRegistradores(v, w, x, y, z);
            int aux_qtdRegistradores = 0;
            if (qtdRegistradores == 1)
            {
                MEM32[R[SP]] = R[aux_push];
                sprintf(instrucao, "push r%d", v);
                fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{0x%08X}={R%d}\n", R[PC], instrucao, R[SP], MEM32[R[SP]], v);
                R[SP] -= 4;
            }
            else if (qtdRegistradores > 0)
            {
                switch (qtdRegistradores)
                {
                case 2:
                    sprintf(instrucao, "push r%d r%d", v, w);
                    break;
                case 3:
                    sprintf(instrucao, "push r%d r%d r%d", v, w, x);
                    break;
                case 4:
                    sprintf(instrucao, "push r%d r%d r%d r%d", v, w, x, y);
                    break;
                case 5:
                    sprintf(instrucao, "push r%d r%d r%d r%d r%d", v, w, x, y, z);
                    break;
                }

                fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{", R[PC], instrucao, R[SP]);

                while (aux_qtdRegistradores < qtdRegistradores)
                {
                    MEM32[R[SP]] = R[aux_push];
                    fprintf(output, "0x%08X", MEM32[R[SP]]);
                    R[SP] -= 4;
                    if (aux_qtdRegistradores + 1 < qtdRegistradores)
                    {
                        fprintf(output, ",");
                        switch (aux_qtdRegistradores)
                        {
                        case 0:
                            aux_push = w;
                            break;
                        case 1:
                            aux_push = x;
                            break;
                        case 2:
                            aux_push = y;
                            break;
                        default:
                            aux_push = z;
                            break;
                        }
                    }
                    aux_qtdRegistradores++;
                }
                fprintf(output, "}={");

                aux_qtdRegistradores = 0;
                aux_push = v;
                while (aux_qtdRegistradores < qtdRegistradores)
                {
                    fprintf(output, "R%d", aux_push);
                    if (aux_qtdRegistradores + 1 < qtdRegistradores)
                    {
                        fprintf(output, ",");
                    }

                    switch (aux_qtdRegistradores)
                    {
                    case 0:
                        aux_push = w;
                        break;
                    case 1:
                        aux_push = x;
                        break;
                    case 2:
                        aux_push = y;
                        break;
                    default:
                        aux_push = z;
                        break;
                    }
                    aux_qtdRegistradores++;
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
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;
            v = (R[28] & (0b11111 << 6)) >> 6;
            w = (R[28] & 0b11111);

            uint8_t aux_pop = v;

            int qtdRegistradores_pop = calcQtdRegistradores(v, w, x, y, z);
            int aux_qtdRegistradores_pop = 0;
            if (qtdRegistradores_pop == 1)
            {
                MEM32[R[SP]] = R[aux_pop];
                sprintf(instrucao, "pop r%d", v);
                fprintf(output, "0x%08X:\t%-25s\t{R%d}=MEM[0x%08X]{0x%08X}\n", R[PC], instrucao, v, R[SP], MEM32[R[SP]]);
                R[SP] += 4;
            }
            else if (qtdRegistradores_pop > 0)
            {
                switch (qtdRegistradores_pop)
                {
                case 2:
                    sprintf(instrucao, "pop r%d r%d", v, w);
                    break;
                case 3:
                    sprintf(instrucao, "pop r%d r%d r%d", v, w, x);
                    break;
                case 4:
                    sprintf(instrucao, "pop r%d r%d r%d r%d", v, w, x, y);
                    break;
                case 5:
                    sprintf(instrucao, "pop r%d r%d r%d r%d r%d", v, w, x, y, z);
                    break;
                }

                fprintf(output, "0x%08X:\t%-25s\t{", R[PC], instrucao);

                while (aux_qtdRegistradores_pop < qtdRegistradores_pop)
                {
                    fprintf(output, "R%d", aux_pop);
                    if (aux_qtdRegistradores_pop + 1 < qtdRegistradores_pop)
                    {
                        fprintf(output, ",");
                    }

                    switch (aux_qtdRegistradores_pop)
                    {
                    case 0:
                        aux_pop = w;
                        break;
                    case 1:
                        aux_pop = x;
                        break;
                    case 2:
                        aux_pop = y;
                        break;
                    default:
                        aux_pop = z;
                        break;
                    }
                    aux_qtdRegistradores_pop++;
                }
                fprintf(output, "}=MEM[0x%08X]{", R[SP]);

                aux_qtdRegistradores_pop = 0;
                aux_pop = v;
                while (aux_qtdRegistradores_pop < qtdRegistradores_pop)
                {
                    R[SP] += 4;
                    fprintf(output, "0x%08X", MEM32[R[SP]]);
                    R[aux_pop] = MEM32[R[SP]];
                    if (aux_qtdRegistradores_pop + 1 < qtdRegistradores_pop)
                    {
                        fprintf(output, ",");
                        switch (aux_qtdRegistradores_pop)
                        {
                        case 0:
                            aux_pop = w;
                            break;
                        case 1:
                            aux_pop = x;
                            break;
                        case 2:
                            aux_pop = y;
                            break;
                        default:
                            aux_pop = z;
                            break;
                        }
                    }
                    aux_qtdRegistradores_pop++;
                }
                fprintf(output, "}\n");
            }
            else
            {
                sprintf(instrucao, "pop -");
                fprintf(output, "0x%08X:\t%-25s\t{}=MEM[0x%08X]{}\n", R[PC], instrucao, R[SP]);
            }

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

            MEM32[R[SP]] = R[PC] + 4;
            R[SP] -= 4;
            R[PC] = (R[x] + (uint32_t)aux_callF) << 1;

            sprintf(instrucao, "call %d", (uint32_t)aux_callF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", R[29], instrucao, R[PC], MEM32[SP], R[SP]);

            break;

        // Instrucao desconhecida
        default:
            fprintf(output, "[INVALID INSTRUCTION @ 0x%08X]\n", R[29]);

            executa = 0;
        }
        R[29] = R[29] + 4;
    }

    fprintf(output, "[END OF SIMULATION]\n");

    fclose(input);
    free(MEM32);
    return 0;
}