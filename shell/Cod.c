#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#define MAX_CMD_LEN 256
#define MAX_ARGS 100
#define HIST_SIZE 10  // Tamanho do histórico

char *history[HIST_SIZE];
int history_index = 0;

// Libera memória do histórico
void liberar_historial() {
    for (int i = 0; i < history_index; i++) {
        free(history[i]);
    }
}

// Adiciona um comando ao histórico, evitando duplicatas ao executar do histórico
void adicionar_ao_historial(const char *comando) {
    if (history_index > 0 && strcmp(history[history_index - 1], comando) == 0) {
        return;  // Evita duplicatas
    }

    if (history_index < HIST_SIZE) {
        history[history_index++] = strdup(comando);
    } else {
        free(history[0]);
        for (int i = 1; i < HIST_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HIST_SIZE - 1] = strdup(comando);
    }
}

// Exibe o histórico de comandos
void exibir_historial() {
    if (history_index == 0) {
        printf("Nenhum comando no histórico.\n");
    } else {
        for (int i = history_index - 1; i >= 0; i--) {
            printf("%d %s\n", i + 1, history[i]);
        }
    }
}

// Verifica se o comando está vazio ou contém apenas espaços
int comando_vazio(const char *comando) {
    while (*comando) {
        if (!isspace(*comando)) return 0;  // Comando não vazio
        comando++;
    }
    return 1;  // Comando vazio
}

// Executa um comando
void executar_comando(char *comando) {
    char *args[MAX_ARGS];
    char *token;
    int i = 0;

    // Remove espaços no início ou no final do comando
    while (*comando == ' ') comando++;
    if (comando_vazio(comando)) return;  // Ignora comandos vazios

    // Comando "exit" libera o histórico antes de sair
    if (strcmp(comando, "exit") == 0) {
        printf("Saindo do shell. Até logo!\n");
        liberar_historial();
        exit(0);
    }

    // Comando de histórico
    if (strcmp(comando, "history") == 0) {
        exibir_historial();
        return;
    }

    // Execução de comando do histórico
    if (strncmp(comando, "!", 1) == 0) {
        if (strcmp(comando, "!!") == 0) {
            if (history_index > 0) {
                char *ultimo_comando = strdup(history[history_index - 1]);
                printf("Executando ultimo comando: %s\n", ultimo_comando);
                adicionar_ao_historial(ultimo_comando);
                executar_comando(ultimo_comando);
                free(ultimo_comando);
            } else {
                printf("Nenhum comando no histórico.\n");
            }
            return;
        } else {
            int index = atoi(&comando[1]);
            if (index > 0 && index <= history_index) {
                printf("Executando comando %d: %s\n", index, history[index - 1]);
                executar_comando(history[index - 1]);
            } else {
                printf("Nenhum comando correspondente no histórico.\n");
            }
            return;
        }
    }

    // Adiciona o comando ao histórico
    adicionar_ao_historial(comando);

    // Quebra o comando em tokens
    token = strtok(comando, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL) return;
 
    // Comando "cd"
    if (strcmp(args[0], "cd") == 0) {
        //Faz uma verificação do comando após o "cd"
	if (i > 1) {
	    //Tenta executar o comando mudando para o diretório especificado
            if (chdir(args[1]) != 0) {
                perror("Erro ao mudar de diretório");
            }
        } else {
	    //Mensagem de erro na má utilização do "cd"
            printf("Uso: cd <diretório>\n");
        }
        return;
    }
 
    // Comando "pwd"
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAX_CMD_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("Erro ao obter diretório atual");
        }
        return;
    }

    // Comando externo
    pid_t pid = fork();
    if (pid == 0) {
        // Processo filho
        if (execvp(args[0], args) == -1) {
            perror("Erro ao executar o comando");
            exit(1);
        }
    } else if (pid > 0) {
        // Processo pai
        wait(NULL);
    } else {
        perror("Erro ao criar o processo");
    }
}

// Função principal
int main() {
    char comando[MAX_CMD_LEN];

    printf("Bem-vindo ao shell aprimorado!\n");

    while (1) {
        printf("osh> ");
        if (fgets(comando, sizeof(comando), stdin) != NULL) {
            comando[strcspn(comando, "\n")] = 0;  // Remove o caractere de nova linha
            executar_comando(comando);
        } else {
            break;  // Termina se a entrada for EOF (Ctrl+D)
        }
    }
}