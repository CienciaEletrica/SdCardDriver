/**
 * @example exemplo_5.c
 * 
 * Terminal de Comandos (Shell) para diagnóstico e manutenção do SD Card.
 * 
 * Este exemplo demonstra como interagir com o sistema de arquivos em tempo real
 * via comandos seriais (UART), permitindo listar arquivos, verificar o espaço 
 * em disco e deletar logs antigos.
 * 
 * @author Tiago Henrique dos Santos / Canal Ciência Elétrica
 * @version 1.0
 * @date 2026-03-30
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "sdcard.h"

/* ==========================================================================
   CONFIGURAÇÕES DA UART (9600 bps)
   ========================================================================== */
#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD - 1)

/** @brief Instâncias globais da biblioteca SD */
SD_Context sd_ctx;
FAT32_Info fat_fs;

/** @brief Buffer para armazenar o comando digitado pelo usuário */
static char cmd_buffer[64];

/**
 * @brief Inicializa a UART para transmissão e recepção (TX/RX).
 */
void uart_init(void) {
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Habilita TX e RX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits, 1 stop bit
}

/**
 * @brief Envia um caractere via UART (usado pelo printf).
 */
int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream);
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

/**
 * @brief Recebe um caractere da UART.
 */
char uart_getchar(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/* ==========================================================================
   FUNÇÕES DO TERMINAL (SHELL)
   ========================================================================== */

/**
 * @brief Lê uma linha inteira de caracteres da UART até encontrar 'Enter'.
 * Faz o "echo" (devolve o caractere) para que o usuário veja o que digita.
 */
void uart_get_line(char *buf, uint8_t n) {
    uint8_t i = 0;
    while (i < n - 1) {
        char c = uart_getchar();
        if (c == '\r' || c == '\n') break; // Fim da linha
        
        // Trata Backspace (apagar caractere)
        if (c == 8 || c == 127) {
            if (i > 0) {
                i--;
                printf("\b \b"); // Apaga visualmente no terminal
            }
        } else {
            putchar(c); // Echo do caractere
            buf[i++] = c;
        }
    }
    buf[i] = '\0'; // Finaliza a string
    putchar('\n');
}

/**
 * @brief Interpreta e executa o comando digitado.
 * 
 * Comandos suportados:
 * - ls: Lista arquivos e tamanhos.
 * - df: Mostra capacidade total do cartão em MB.
 * - rm "NOME": Deleta o arquivo especificado. Ex.: "rm CONFIG.INI"
 */
void processar_comando(char *cmd) {
    SD_FileName file;
    
    // Comando: ls (Listar diretório)
    if (strcmp(cmd, "ls") == 0) {
        printf("\n--- CONTEUDO DO CARTAO SD ---\n");
        sd_dir_list(&sd_ctx, &fat_fs);
    } 
    // Comando: df (Disk Free / Informação de capacidade)
    else if (strcmp(cmd, "df") == 0) {
        uint32_t cap = sd_get_capacity_mb(&sd_ctx);
        printf("\nCapacidade Total do Cartao: %lu MB\n", cap);
    }
    // Comando: rm (Remove / Deletar arquivo)
    else if (strncmp(cmd, "rm ", 3) == 0) {
        char *nome = cmd + 3; // Pula os 3 primeiros caracteres ("rm ")
        sd_set_filename(&file, nome);
        
        if (sd_file_delete(&sd_ctx, &fat_fs, &file) == SD_SUCCESS) {
            printf("Arquivo '%s' removido com sucesso.\n", nome);
        } else {
            printf("Erro: Nao foi possivel remover '%s' (Arquivo nao existe?)\n", nome);
        }
    }
    else if (strlen(cmd) > 0) {
        printf("Comando desconhecido!\n");
        printf("Use: ls, df, ou rm <ARQUIVO>\n");
    }
}

/**
 * @brief Função de delay exigida pela biblioteca sd_begin.
 */
void my_delay_ms_lib(uint32_t ms) {
    while (ms--) _delay_ms(1);
}

/* ==========================================================================
   PROGRAMA PRINCIPAL
   ========================================================================== */

int main(void) {
    uart_init();
    stdout = &mystdout;
    _delay_ms(1000);

    printf("\n=== AVR SD SHELL v1.0 ===\n");
    printf("Canal Ciencia Eletrica - Tiago Henrique\n");

    // Inicialização do hardware
    if (sd_begin(&sd_ctx, my_delay_ms_lib) == SD_SUCCESS) {
        if (sd_fat_mount(&sd_ctx, &fat_fs) == SD_SUCCESS) {
            
            printf("[SISTEMA] SD Card pronto. Digite comandos.\n");

            while (1) {
                printf("\nAVR-SD> "); // Prompt do terminal
                uart_get_line(cmd_buffer, sizeof(cmd_buffer));
                processar_comando(cmd_buffer);
            }
            
        } else {
            printf("[ERRO] Erro ao montar FAT32!\n");
        }
    } else {
        printf("[ERRO] Cartao SD nao detectado!\n");
    }

    while (1); // Trava aqui em caso de falha crítica
    return 0;
}

