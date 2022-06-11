# VIRTUAL MEMORY
> A atividade passada na cadeira de Infraestrutura de sofware se baseia na simulação de uma memoria virtual abordada no livro Operating System Concepts p-51
> A dinamica consiste em ler um arquivo txt com endereços e simular uma pagetable, tlb, memoria fisica, entre outras ferramentas.
> Essa implementação foi feita na linguagem C

## main.c
> Arquivo em que toda a implementação se encontra

## makefile
> fundamental para a compilação do código
> $make
>compila o codigo e cria o binario
>$make clean
>exclui o binario
>$make run
>executa o codigo

## addresses.txt
>arquivo lido pelo código, o qual contém vários endereços virtuais

## BACKING_STORE.bin
>arquivo binário que será lido para coletar os diversos offsets das paginas trabalhadas

## correct.txt
> arquivo que será criado pelo codigo com informações(endereço virtual, endereço fisico, valor)de acordo com os endereços lidos
> no fim do arquivo mostra a quantidade de pagefaults e de tlbhits que ocorreram ao longo do processo

# Formato para executar o código
> ./vm arquivo.txt algoritmo algoritmo
> sendo arquivo.txt o arquivo a ser lido, algoritmo um dos algoritmos de substituição (fifo / lru) 

