#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    FILE *input = fopen("1_erro.hex", "r");
    if (input == NULL)
    {
        perror("Erro ao abrir o arquivo 1_erro.hex");
        return 1;
    }

    uint32_t R[32] = { 0 };
    uint8_t* MEM8 = (uint8_t*)(calloc(32, 1024));
    uint32_t *MEM32 = (uint32_t *)calloc(32 * 1024, sizeof(uint32_t));

    if (MEM32 == NULL)
    {
        perror("Erro ao alocar MEM32");
        fclose(input);
        return 1;
    } 
 
    int i = 0;
    
      while (fscanf(input, "%x", &MEM32[i]) == 1) {
        // Preenche MEM8 com os bytes do MEM32
        MEM8[i * 4 + 0] = (MEM32[i] >> 24) & 0xFF;
        MEM8[i * 4 + 1] = (MEM32[i] >> 16) & 0xFF;
        MEM8[i * 4 + 2] = (MEM32[i] >> 8) & 0xFF;
        MEM8[i * 4 + 3] = (MEM32[i]) & 0xFF;

        i++;
    }
 
 
    uint8_t executa = 1;

    // while(executa) {
	// 	// Cadeia de caracteres da instrucao
	// 	char instrucao[30] = { 0 };
	// 	// Declarando operandos
	// 	uint8_t z = 0, x = 0, i = 0;
	// 	uint32_t pc = 0, xyl = 0;
	// 	// Carregando a instrucao de 32 bits (4 bytes) da memoria indexada pelo PC (R29) no registrador IR (R28)
	// 	// E feita a leitura redundante com MEM8 e MEM32 para mostrar formas equivalentes de acesso
	// 	// Se X (MEM8) for igual a Y (MEM32), entao X e Y sao iguais a X | Y (redundancia)
	// 	R[28] = ((MEM8[R[29] + 0] << 24) | (MEM8[R[29] + 1] << 16) | (MEM8[R[29] + 2] << 8) | (MEM8[R[29] + 3] << 0)) | MEM32[R[29] >> 2];
	// 	// Obtendo o codigo da operacao (6 bits mais significativos)
	// 	uint8_t opcode = (R[28] & (0b111111 << 26)) >> 26;
	// 	// Decodificando a instrucao buscada na memoria
	// 	switch(opcode) {
	// 		// mov
	// 		case 0b000000:
	// 			// Obtendo operandos
	// 			z = (R[28] & (0b11111 << 21)) >> 21;
	// 			xyl = R[28] & 0x1FFFFF;
	// 			// Execucao do comportamento
	// 			R[z] = xyl;
	// 			// Formatacao da instrucao
	// 			sprintf(instrucao, "mov r%u,%u", z, xyl);
	// 			// Formatacao de saida em tela (deve mudar para o arquivo de saida)
	// 			printf("0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, xyl);
	// 			break;
	// 		// l8
	// 		case 0b011000:
	// 			// Otendo operandos
	// 			z = (R[28] & (0b11111 << 21)) >> 21;
	// 			x = (R[28] & (0b11111 << 16)) >> 16;
	// 			i = R[28] & 0xFFFF;
	// 			// Execucao do comportamento com MEM8 e MEM32 (calculo do indice da palavra e selecao do byte big-endian)
	// 			R[z] = MEM8[R[x] + i] | (((uint8_t*)(&MEM32[(R[x] + i) >> 2]))[3 - ((R[x] + i) % 4)]);
	// 			// Formatacao da instrucao
	// 			sprintf(instrucao, "l8 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
	// 			// Formatacao de saida em tela (deve mudar para o arquivo de saida)
	// 			printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + i, R[z]);
	// 			break;
	// 		// l32
	// 		case 0b011010:
	// 			// Otendo operandos
	// 			z = (R[28] & (0b11111 << 21)) >> 21;
	// 			x = (R[28] & (0b11111 << 16)) >> 16;
	// 			i = R[28] & 0xFFFF;
	// 			// Execucao do comportamento com MEM8 e MEM32
	// 			R[z] = ((MEM8[((R[x] + i) << 2) + 0] << 24) | (MEM8[((R[x] + i) << 2) + 1] << 16) | (MEM8[((R[x] + i) << 2) + 2] << 8) | (MEM8[((R[x] + i) << 2) + 3] << 0)) | MEM32[R[x] + i];
	// 			// Formatacao da instrucao
	// 			sprintf(instrucao, "l32 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
	// 			// Formatacao de saida em tela (deve mudar para o arquivo de saida)
	// 			printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%08X\n", R[29], instrucao, z, (R[x] + i) << 2, R[z]);
	// 			break;
	// 		// bun
	// 		case 0b110111:
	// 			// Armazenando o PC antigo
	// 			pc = R[29];
	// 			// Execucao do comportamento
	// 			R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);
	// 			// Formatacao da instrucao
	// 			sprintf(instrucao, "bun %i", R[28] & 0x3FFFFFF);
	// 			// Formatacao de saida em tela (deve mudar para o arquivo de saida)
	// 			printf("0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
	// 			break;
	// 		// int
	// 		case 0b111111:
	// 			// Parar a execucao
	// 			executa = 0;
	// 			// Formatacao da instrucao
	// 			sprintf(instrucao, "int 0");
	// 			// Formatacao de saida em tela (deve mudar para o arquivo de saida)
	// 			printf("0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);
	// 			break;
	// 		// Instrucao desconhecida
	// 		default:
	// 			// Exibindo mensagem de erro
	// 			printf("Instrucao desconhecida!\n");
	// 			// Parar a execucao
	// 			executa = 0;
	// 	}
	// 	// PC = PC + 4 (proxima instrucao)
	// 	R[29] = R[29] + 4;
	// }


    fclose(input);
    free(MEM32);
    return 0;
}