#include<stdio.h>

int main() {
    FILE *fd;
    
    //////////////////////////////
    struct segurados {
        char cod_cli[4];
        char nome_cli[50];
        char nome_seg[50];
        char tipo_seg[30];
    } vet[10] = {{"001", "Maria", "Porto", "Residencia"},
                {"002", "Joao", "Zurich", "Auto"},
                {"003", "Pedro", "Porto", "Vida"},
                {"004", "Gabriel", "Chagas", "Teste"},
                {"005", "Mariana", "Damasceno", "Pazianoto"},
                {"006", "ADS", "ASD", "ASD"},
                {"007", "Sete", "Sete", "Sete"},
                {"008", "Oito", "Oito", "Oito"},
                {"009", "Nove", "Nove", "Nove"},
                {"010", "Dez", "Dez", "Dez"}};
       
    fd = fopen("insere.bin", "w+b");
    fwrite(vet, sizeof(vet), 1, fd);
    fclose(fd);
    
    //////////////////////////////
	struct remove {
        char cod_cli[4];
    } vet_r[4] = {{"003"},{"002"},{"001"},{"010"}};
       
    fd = fopen("remove.bin", "w+b");
    fwrite(vet_r, sizeof(vet_r), 1, fd);
    fclose(fd);
}

