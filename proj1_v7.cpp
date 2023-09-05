#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

struct registro_t {
	char codSeg[4];
	char nomeSeg[50];
	char seguradora[50];
	char tipoSeg[30];
};

struct key_t {
	char codSeg[4];
};

struct header_t {
	int lidos_insere;
	int lidos_remove;
	int ponteiro;
};

int pega_registro(FILE *fp, char *buffer) {
	int hashes = 0, tam = 0, i = 0;

	
	while(hashes < 4) {
		buffer[i] = fgetc(fp);
		if (buffer[i] == '#')
			hashes++;
		tam++;
		i++;
	}
	
	
	buffer[i] = '\0';
	return tam;
}

void inserir (FILE **fp_insere, FILE *fp_arquivo) {
	if ((*fp_insere = fopen("insere.bin","r+b")) == NULL) {	//Abre o arquivo insere.bin no modo leitura como arquivo binário
		printf("Nao foi possivel abrir o arquivo");
		return;
	}
	
	struct registro_t registro;	//Cria estrutura do registro
	struct header_t header; //Cria estrutura do cabeçalho
	
	rewind(fp_arquivo);	//Certifica que o ponteiro esteja no começo do arquivo
	fread(&header, sizeof(header), 1, fp_arquivo);	//Le informações do cabeçalho
	
	fseek(*fp_insere, header.lidos_insere*sizeof(registro), 0);	//Posiciona o ponteiro do arquivo insere.bin na posição dos arquivos não lidos até o momento.
	fread(&registro, sizeof(registro), 1, *fp_insere); //Le o registro de insere.bin
	
	char buffer[512];	//Cria um buffer para o registro
	sprintf(buffer, "%s#%s#%s#%s#", registro.codSeg, registro.nomeSeg, registro.seguradora, registro.tipoSeg); //Insere no buffer as informações lidas de insere.bin
	
	if (feof(*fp_insere)){	//Se o arquivo insere.bin está no final do arquivo 
        printf("\nNao ha mais registros a inserir\n");	//Indica que não há mais registros
        return; //E retorna para que não seja inserido no arquivo lixo de memória
    }
	
	int tam_reg = strlen(buffer);	//Calcula o tamanho do registro a partir do tamanho do buffer
	int espaco_livre;	//Cria variavel de espaco livre
	
	int espaco_livre_anterior = 8; //Byte Offset do espaço livre anterior calculado antes da inserçao, 
									//8 bytes para pular o cabeçalho de lidos_insere e lidos_remove
	int espaco_livre_atual; //Byte Offset do espaço livre atual que será calculado após inserção
	int proximo_espaco_livre;	//Byte Offset até o próximo espaço livre
	
	if (header.ponteiro != -1) {	//Se o ponteiro da lista do cabeçalho quye indica o Byte Offset do espaço livre for diferente de -1 quer dizer que há espaço livre no arquivo
		fseek(fp_arquivo, header.ponteiro, 0);	//Posiciona o ponteiro do arquivo principal para o Byte Offset do ponteiro da lista do cabeçalho
		espaco_livre_atual = header.ponteiro;	//Espaco livre atual é onde o ponteiro da lista do cabeçalho apontava.
		
		while (1) {	//Laço Infinito
			fread(&espaco_livre, sizeof(int), 1, fp_arquivo); //Le o tamanho do espaco livre pois o ponteiro do arquivo principal
															  //já está no byte que indica o tamanho do espaço livre
			fgetc(fp_arquivo);	//Le um caracter para ignorar o #
			fread(&proximo_espaco_livre, sizeof(int), 1, fp_arquivo); //Guarda o Byte Offset para o próximo espaço livre
			
			if (tam_reg <= espaco_livre) {	//Se o tamanho do registro a ser inserido cabe no espaco livre atual calculado
				fseek(fp_arquivo, (-1)*sizeof(int) - 1, 1);	//Faz o deslocamento inverso do ponteiro após ler o proximo espaço livre disponivel 
				fwrite(buffer, 1, tam_reg, fp_arquivo);		//Escreve o registro no espaço livre
				
				if (espaco_livre_anterior == 8) {	//Se o espaco livre anterior ainda era o primeiro da lista, isto é, espaço livre após o cabeçalho (8 bytes pulados do cabeçalho)
					header.ponteiro = proximo_espaco_livre;	//Ponteiro da lista do cabeçalho é atualizado para guardar o Byte offset do próximo espaço livre
					rewind(fp_arquivo);	//Volta o ponteiro do arquivo para o início
					fwrite(&header, sizeof(header), 1, fp_arquivo); //Atualiza as informações do cabeçalho
				}
				else {
					fseek(fp_arquivo, espaco_livre_anterior + sizeof(int) + 1, 0); //Se o espaco livre anterior não é o primeiro da lista, posiciona o ponteiro depois dos primerios 8 bytes do cabeçalho + 4bytes + 1
					fwrite(&proximo_espaco_livre, 1, sizeof(int), fp_arquivo);	//Para escrever e atualizar no cabeçalho qual vai ser o proximo espaco livre.
				}
				
				break;	//Sai do laço infinito
			}
			
			else {	//Se o tamanho do registro nao cabe no espaco livre calculado
				if (proximo_espaco_livre == -1) {	//Se nao ha mais espacos livres do tamanho calculado do registro
					fseek(fp_arquivo, 0, 2);	//Posiciona ponteiro no final do arquivo
					fwrite(&tam_reg, sizeof(int), 1, fp_arquivo);	//Insere o tamanho do registro
					fwrite(buffer, tam_reg, 1, fp_arquivo); //E por fim insere o registro no final
					break;	//Sai do laço infinito.
				}
				
				else {	//Se ainda há espaço livre, isto é, proximo espaco livre possui um valor diferente de -1.
					if (espaco_livre_anterior == 8) {	//Se o espaco livre anterior ainda era o primeiro da lista (8 bytes pulados do cabeçalho)
						header.ponteiro = espaco_livre_atual; //Atualiza o ponteiro da lista do cabeçalho para o Byte Offset do espaco livre atual
						rewind(fp_arquivo);	//Volta o ponteiro do arquivo para o começo
						fwrite(&header, sizeof(header), 1, fp_arquivo); //E atualiza o cabeçalho
					}
					
					else { //Se o espaco livre anterior não era o primeiro da lista
						fseek(fp_arquivo, espaco_livre_anterior + sizeof(int) + 1, 0); //Posiciona o ponteiro depois dos primerios 8 bytes do cabeçalho + 4bytes + 1
						fwrite(&espaco_livre_atual, 1, sizeof(int), fp_arquivo); //Para escrever no cabeçalho o Byte Offset espaco livre atual.
					} 
					
					fseek(fp_arquivo, espaco_livre_atual + sizeof(int) + 1, 0); //Posiciona o ponteiro no Byte Offset do espaco livre atual + 4bytes + 1
					fwrite(&proximo_espaco_livre, sizeof(int), 1, fp_arquivo); //Escreve o byte offset do proximo espaco livre 
					
					espaco_livre_anterior = espaco_livre_atual; //Espaco livre anterior se torna o byteoffset do novo espaco livre calculado	
					espaco_livre_atual = proximo_espaco_livre; //O espaco livre atual se torna o byteoffset do proximo espaco livre encontrado 
					fseek(fp_arquivo, proximo_espaco_livre, 0); //Posiciona o ponteiro do arquivo no byte offset do proximo espaco livre
				}
				
			}
			
		} 
	}
	
	else {	//Se nao há espaço livre no arquivo
		fseek(fp_arquivo, 0, 2);	//Posiciona o ponteiro no final do arquivo
		fwrite(&tam_reg, sizeof(int), 1, fp_arquivo);	//Escreve o tamanho do registro
		fwrite(buffer, tam_reg, 1, fp_arquivo); //Finalmente escreve o registro.
	}
	
	header.lidos_insere++;	//Atualiza o numero de registros de inserção já lidos
		
	rewind(fp_arquivo); //Volta o ponteiro para o inicio do arquivo
	fwrite(&header, sizeof(header), 1, fp_arquivo); //Atualiza informações do cabeçalho
	
	printf("\nRegistro incluido com sucesso!\n");
	
	fclose(*fp_insere);	//Fecha o arquivo insere.bin
}

void remover(FILE **fp_remove, FILE *fp_arq) {
	if ((*fp_remove = fopen("remove.bin", "r+b")) == NULL) {	//Abre o arquivo de remoção remove.bin em modo leitura e como arquivo binário.
		printf("Nao foi possivel abrir o arquivo");
		return;
	}
	
	char key[3];	//Cria estrutura de chave
	struct header_t header;	//Cria estrutura de cabeçalho
	
	int tam_reg = 0;	//Variável de tamanho do registrador
	int bytes_offset_sum = sizeof(header);	//Somatorio de bytes offset do tamanho do cabeçalho
	int reg_checks = 0;	//registros checados
	char buffer_key[256];	//buffer para armazenar as chaves
	char buffer_reg[512];	//buffer para armazenar os registros
	
	rewind(fp_arq);		//Certifica que o ponteiro do arquivo principal esteja no inicio do arquivo
	fread(&header, sizeof(header), 1, fp_arq);	//Le informacoes do cabeçalho
	
	fseek(*fp_remove, header.lidos_remove*sizeof(key), 0);	//Move o ponteiro do arquivo de remoção para a posição das chaves dos registros nao lidos
	fread(&key, sizeof(key), 1, *fp_remove);	//Le a chave (codSeg) do arquivo de remocao
	sprintf(buffer_key, "%s", key);	//Escreve no buffer de chaves o código a ser removido
	
	while (1) {	//Laço infinito
		fread(&tam_reg, sizeof(int), 1, fp_arq);	//Le o tamanho do registro do arquivo principal
		fread(&buffer_reg, tam_reg, 1, fp_arq);		//Le um registro de tam_reg no arquivo principal e salva no buffer de registros
		
		if (buffer_reg[0] != buffer_key[0] || buffer_reg[1] != buffer_key[1] || buffer_reg[2] != buffer_key[2]) {	//Se a primeira ou a segunda ou a terceira posicao do Código Chave [000] 
																									//forem diferentes quer dizer que o código de remoção não corresponde ao registro bufferizado
			bytes_offset_sum += tam_reg;			//Acumula o tamanho do registro procurado para saber o byte offset para percorrer o arquivo
			reg_checks++;							//Aumenta o contador de registros checados
				
			fgetc(fp_arq);							//Le caracter a caracter do arquivo principal
			if (feof(fp_arq)) {						//Se chegou ao fim do arquivo principal
				header.lidos_remove++;				//Incrementa o número de registros de remoção lidos
				rewind(fp_arq);						//Volta o ponteiro do arquivo principal para o inicio
				fwrite(&header, sizeof(header), 1, fp_arq);		//Atualiza as informações do cabeçalho
				
				printf("\nNao ha registro correspondente\n");		//Indica que nao ha o codigo chave daquele registro no arquivo principal
				
				fclose(*fp_remove);	//Fecha arquivo de remocao remove.bin
				
				return;	//Retorna para o menu
			}
			
			else	//Se não é o fim do arquivo desloca um byte para trás no arquivo a partir da posicao atual do ponteiro
				fseek(fp_arq, -1, 1);
		}
		
		else break;	//Se o código chave do buffer de chaves corresponde a um registro bufferizado sai do laço infinito
	}
	
	fseek(fp_arq, (-1)*tam_reg, 1);	//Desloca o ponteiro voltando para o inicio daquele registro que corresponde ao codigo chave voltando o offset do tamanho do registro
	char rm = '*';	
	fwrite(&rm, 1, 1, fp_arq);	//Marca o registro excluido com o caracter *
	fwrite(&header.ponteiro, sizeof(int), 1, fp_arq);	//Escreve o ponteiro da cabeça da lista do cabeçalho que contem o byte offset da posicao no arquivo para indicar como espaço livre
	
	bytes_offset_sum += reg_checks*sizeof(int); //O somatorio dos bytes offsets é igual ao numero de registros checados * 4 bytes.
	
	header.ponteiro = bytes_offset_sum; //Ponteiro da lista do cabeçalho aponta para depois do registro excluido
	header.lidos_remove++;	//Incrementa registros removidos já lidos
	
	rewind(fp_arq);	//Volta ponteiro do arquivo principal para o inicio
	fwrite(&header, sizeof(header), 1, fp_arq);	//Atualiza informacoes do cabeçalho
	
	printf("\nRegistro excluido com sucesso!\n");	
	
	fclose(*fp_remove);	//Fecha arquivo de remoçao remove.bin
}

void compactar (FILE *fp_arq){
	int tam_espaco, tam_reg;
	char buffer[512];
	char ch_aux;
	
	struct header_t header;
	
	FILE *fp_aux;
	
	if ((fp_aux = fopen("arq_auxiliar.bin", "w+b")) == NULL) {
		printf("Nao foi possivel abrir o arquivo");
		return;
	}
	
	rewind(fp_arq);
	fread(&header, sizeof(header), 1, fp_arq);
	header.ponteiro = -1;
	fwrite(&header, sizeof(header), 1, fp_aux);
	
	while (1) {
		
		fread(&tam_espaco, sizeof(int), 1, fp_arq);
		ch_aux = fgetc(fp_arq);
		
		
		
		if (ch_aux == '*') {
			fseek(fp_arq, (tam_espaco - 1), 1);
			continue;
		}
		
		if (ch_aux == EOF)
			break;
		
			
		fseek(fp_arq, -1, 1);
		tam_reg = pega_registro(fp_arq, buffer);
		
		
		fwrite(&tam_reg, sizeof(int), 1, fp_aux);
		fwrite(buffer, strlen(buffer), 1, fp_aux);
		
		fseek(fp_arq, tam_espaco - tam_reg, 1);
		ch_aux = fgetc(fp_arq);
		
		
		
		if (ch_aux != EOF)
			fseek(fp_arq, -1, 1);
		else
			break;	
	}
	
	fclose(fp_arq);
	fclose(fp_aux);
	
	char arquivo_original[100] = "arq_registros.bin";
	char arquivo_auxiliar[100] = "arq_auxiliar.bin";
	
	if (remove(arquivo_original) == -1)
		printf("\nErro ao excluir arquivo!\n");
		
	if (rename(arquivo_auxiliar, arquivo_original) != 0)
		printf("\nErro ao renomear arquivo\n");
	
	remove(arquivo_auxiliar);
	
	printf("\nArquivo compactado com sucesso!\n");
	
	if ((fp_arq = fopen("arq_registros.bin", "r+b")) == NULL) {
		printf("Nao foi possivel abrir o arquivo");
		return;
	}
}

int main() {
	FILE *fp_arquivo, *fp_insere, *fp_remove;
	
	struct header_t header;
	
	header.lidos_insere = 0;
	header.lidos_remove = 0;
	header.ponteiro = -1;
	
	if (access("arq_registros.bin", F_OK) == 0) {
		if((fp_arquivo = fopen("arq_registros.bin","r+b")) == NULL){
			printf("Nao foi possivel abrir o arquivo");
			return 0;
		}
	}
	else {
		if ((fp_arquivo = fopen("arq_registros.bin","w+b")) == NULL) {
			printf("Nao foi possivel abrir o arquivo");
			return 0;
		}
		
		fwrite(&header, sizeof(header), 1, fp_arquivo);
		rewind(fp_arquivo);	
	}
	
	int op = 0;
	
	while (op < 4) {
	
		do {
			printf("\n----------------Menu------------------\n");
			printf("Inserir [1]\n");
			printf("Remover [2]\n");
			printf("Compactar [3]\n");
			printf("Sair [4]\n");
			printf("Opcao: ");
			scanf(" %d", &op);
		} while (op != 1 && op != 2 && op != 3 && op != 4);
	
		switch(op) {
			case 1: inserir(&fp_insere, fp_arquivo);
					break;
			case 2: remover(&fp_remove, fp_arquivo);
					break;
			case 3: compactar(fp_arquivo);
					break;
			case 4: return 0;
					break;
			default: break;
		}
	}
	
}
