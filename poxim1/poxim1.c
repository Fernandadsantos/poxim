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

#define IR 28
#define PC 29
#define SP 30
#define SR 31

char *strRegistrador(int num, int type)
{
    char *result = (char *)malloc(10 * sizeof(char));
    if (result == NULL)
    {
        return NULL;
    }

    if (num < 28 || num > 31)
    {
        snprintf(result, 10, "%c%d", type ? 'r' : 'R', num);
        return result;
    }

    switch (num)
    {
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

int main(int argc, char *argv[])
{
    FILE *input = fopen(argv[1], "r");
    FILE *output = fopen(argv[2], "w");
    // FILE *input = fopen("input.txt", "r");
    // FILE *output = fopen("output.txt", "w");
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
        int32_t auxSigned32 = 0, aux_mask = 0;
        int64_t auxSigned64 = 0;
        uint64_t auxUnsigned64 = 0;

        R[28] = MEM32[R[29] >> 2];

        uint8_t opcode = (R[28] & (0b111111 << 26)) >> 26;

        switch (opcode)
        {
        // mov
        case 0b000000:
            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            R[z] = xyl;

            sprintf(instrucao, "mov %s,%u", strRegistrador(z, 1), xyl);
            fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), xyl);
            break;

        // divI
        case 0b010101:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            int16_t aux_divi = R[28] & 0xFFFF;
            auxSigned32 = (int32_t)aux_divi;
            int32_t auxRz = 0;

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
            }
            else
            {
                auxRz = 0;

                if (auxRz == 0)
                {
                    R[31] |= ZN;
                }
                else
                {
                    R[31] &= ~ZN;
                }

                if (auxSigned32 == 0)
                {
                    R[31] |= ZD;
                }
                else
                {
                    R[31] &= ~ZD;
                }
            }

            sprintf(instrucao, "divi %s,%s,%d", strRegistrador(z, 1), strRegistrador(x, 1), auxSigned32);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), auxSigned32, R[z], R[31]);
            break;

        // movs
        case 0b000001:

            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            auxSigned32 = (R[28] >> 20) & 1;
            aux_mask = auxSigned32 ? 0xFFE00000 : 0x00000000;
            auxSigned32 = aux_mask | xyl;

            R[z] = auxSigned32;

            sprintf(instrucao, "movs %s,%d", strRegistrador(z, 1), auxSigned32);

            fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), R[z]);
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

            sprintf(instrucao, "add %s,%s,%s", strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(y, 1));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(y, 0), R[z], R[31]);
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

            sprintf(instrucao, "addi %s,%s,%d", strRegistrador(z, 1), strRegistrador(x, 1), (int32_t)aux_addI);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s+0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), (int32_t)aux_addI, R[z], R[31]);

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

            sprintf(instrucao, "sub %s,%s,%s", strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(y, 1));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s-%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(y, 0), R[z], R[31]);

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

            sprintf(instrucao, "subi %s,%s,%d", strRegistrador(z, 1), strRegistrador(x, 1), (int32_t)aux_i);

            fprintf(output, "0x%08X:\t%-25s\t%s=%s-0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), (int32_t)aux_i, R[z], R[31]);
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

                sprintf(instrucao, "mul %s,%s,%s,%s", strRegistrador(l, 1), strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(y, 1));
                fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%016" PRIX64 ",SR=0x%08X\n", R[29], instrucao, strRegistrador(l, 0), strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(y, 0), auxUnsigned64, R[31]);
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

                sprintf(instrucao, "sll %s,%s,%s,%u", strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(x, 1), l);
                fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%016" PRIX64 ",SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(z, 0), strRegistrador(x, 0), l + 1, auxUnsigned64, R[31]);
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

                    if (R[l] != 0)
                    {
                        R[31] |= CY;
                    }
                    else
                    {
                        R[31] &= ~CY;
                    }
                }
                else
                {

                    if (R[z] == 0)
                    {
                        R[31] |= ZN;
                    }
                    else
                    {
                        R[31] &= ~ZN;
                    }

                    if (R[y] == 0)
                    {
                        R[31] |= ZD;
                    }
                    else
                    {
                        R[31] &= ~ZD;
                    }
                }
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

            sprintf(instrucao, "modi r%u,r%u,%d", z, x, aux_modi);
            fprintf(output, "0x%08X:\t%-25s\tR%u=R%u%%0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, z, x, aux_modi, R[z], R[31]);

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

            sprintf(instrucao, "cmp %s,%s", strRegistrador(x, 1), strRegistrador(y, 1));
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

            sprintf(instrucao, "cmpi %s,%d", strRegistrador(x, 1), aux_cmpi);
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

            sprintf(instrucao, "and %s,%s,%s", strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(y, 1));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(y, 0), R[z], R[31]);

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

            sprintf(instrucao, "or %s,%s,%s", strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(y, 1));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(y, 0), R[z], R[31]);

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

            sprintf(instrucao, "not %s,%s", strRegistrador(z, 1), strRegistrador(x, 1));
            fprintf(output, "0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), R[z], R[31]);

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

            sprintf(instrucao, "xor %s,%s,%s", strRegistrador(z, 1), strRegistrador(x, 1), strRegistrador(y, 1));
            fprintf(output, "0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n", R[29], instrucao, strRegistrador(z, 0), strRegistrador(x, 0), strRegistrador(y, 0), R[z], R[31]);

            break;

        // l8
        case 0b011000:

            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_l8 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_l8;

            R[z] = (((uint8_t *)(&MEM32[(R[x] + auxUnsigned32) >> 2]))[3 - ((R[x] + auxUnsigned32) % 4)]);

            sprintf(instrucao, "l8 r%u,[r%u%s%i]", z, x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + auxUnsigned32, R[z]);

            break;

        // l16
        case 0b011001:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_l16 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_l16;

            R[z] = ((uint16_t *)(&MEM32[(R[x] + auxUnsigned32) >> 1]))[1 - ((R[x] + auxUnsigned32) % 2)];

            sprintf(instrucao, "l16 r%u,[r%u%s%i]", z, x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32);
            fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%04X\n", R[29], instrucao, z, (R[x] + auxUnsigned32) << 1, R[z]);

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
            uint16_t aux_s8 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_s8;

            (((uint8_t *)(&MEM32[(R[x] + auxUnsigned32) >> 2]))[3 - ((R[x] + auxUnsigned32) % 4)]) = R[z];

            sprintf(instrucao, "s8 [r%u%s%i],r%u", x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32, z);
            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%02X\n", R[29], instrucao, R[x] + auxUnsigned32, z, R[z]);

            break;

        // s16
        case 0b011100:
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            uint16_t aux_s16 = R[28] & 0xFFFF;
            auxUnsigned32 = (uint32_t)aux_s16;

            ((uint16_t *)(&MEM32[(R[x] + auxUnsigned32) >> 1]))[1 - ((R[x] + auxUnsigned32) % 2)] = R[z];

            sprintf(instrucao, "s16 [r%u%s%i],r%u", x, (auxUnsigned32 >= 0) ? ("+") : (""), auxUnsigned32, z);

            fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=R%u=0x%04X\n", R[29], instrucao, (R[x] + auxUnsigned32) << 1, z, R[z]);

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
            auxSigned32 = (R[28] & 0x3FFFFFF);

            if ((R[31] & 1 << 3) != (R[31] & 1 << 4))
            {
                R[29] = R[29] + (auxSigned32 << 2);
            }

            sprintf(instrucao, "blt %i", R[28] & 0x3FFFFFF);
            fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);

            break;

        // bne
        case 0b110100:
            auxSigned32 = (R[28] & 0x3FFFFFF);

            if (!(R[31] & 1 << 6))
            {
                R[29] = R[29] + (auxSigned32 << 2);
            }

            sprintf(instrucao, "bne %i", R[28] & 0x3FFFFFF);
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

            R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);

            sprintf(instrucao, "bun %i", R[28] & 0x3FFFFFF);
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
            executa = 0;
            sprintf(instrucao, "int 0");
            fprintf(output, "0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);

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
                    "push r%d",
                    "push r%d,r%d",
                    "push r%d,r%d,r%d",
                    "push r%d,r%d,r%d,r%d",
                    "push r%d,r%d,r%d,r%d,r%d"};

                sprintf(instrucao, formatosPush[qtdRegistradores], regsPush[0], regsPush[1], regsPush[2], regsPush[3], regsPush[4]);
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
                    fprintf(output, "R%d", regsPush[i]);
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
                    "pop r%d",
                    "pop r%d,r%d",
                    "pop r%d,r%d,r%d",
                    "pop r%d,r%d,r%d,r%d",
                    "pop r%d,r%d,r%d,r%d,r%d"};

                sprintf(instrucao, formatosPop[qtdRegistradores_pop], regsPop[0], regsPop[1], regsPop[2], regsPop[3], regsPop[4]);
                fprintf(output, "0x%08X:\t%-25s\t{", R[PC], instrucao);

                for (int i = 0; i < qtdRegistradores_pop; i++)
                {
                    fprintf(output, "R%d", regsPop[i]);
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

            executa = 0;
        }
        R[29] = R[29] + 4;
    }

    fprintf(output, "[END OF SIMULATION]\n");

    fclose(input);
    fclose(output);
    free(MEM32);
    return 0;
}