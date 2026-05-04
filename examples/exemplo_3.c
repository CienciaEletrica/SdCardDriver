/**
 * @example exemplo_3.c
 * 
 * Sistema de Datalogger com Rotação Automática de Arquivos e Timestamp.
 * Este exemplo demonstra como gerenciar o armazenamento de forma profissional em cartões SD.
 * O sistema monitora o tamanho do arquivo atual e, ao atingir um limite (2,5KB), cria 
 * automaticamente um novo volume (LOG001.TXT, LOG002.TXT, etc). 
 * Inclui carimbo de data/hora (Timestamp) simulado para cada registro.
 * 
 * @author Tiago Henrique dos Santos / Canal Ciência Elétrica
 * @version 1.0
 * @date 2026-03-30
 */

#define F_CPU 16000000UL /**< Frequência de operação para delays e UART */

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdcard.h"

/* ==========================================================================
   CONFIGURAÇÕES E CONSTANTES
   ========================================================================== */

#define TAMANHO_MAX_BYTES    2560  /**< Limite de 2,5KB por arquivo para teste rápido */
#define MAX_VOLUMES          99    /**< Limite de segurança: LOG099.TXT */
#define BAUD                 9600  /**< Velocidade da Serial */
#define MYUBRR               (F_CPU/16/BAUD - 1)

/* ==========================================================================
   VARIÁVEIS GLOBAIS
   ========================================================================== */

SD_Context sd_ctx;   /**< Contexto de hardware do SD */
FAT32_Info fat_fs;   /**< Informações do sistema de arquivos */

static char log_buffer[128];        /**< Buffer para formatar a linha CSV */
static char nome_arquivo_ativo[13]; /**< Armazena o nome do arquivo atual (8.3) */

/**
 * @brief Estrutura para Simulação de Tempo (Timestamp).
 * Em projetos reais, os dados devem ser lidos de um RTC (ex: DS3231).
 */
typedef struct {
    uint8_t dia, mes, ano;
    uint8_t hora, min, seg;
} Timestamp;

/* ==========================================================================
   COMUNICAÇÃO SERIAL (UART)
   ========================================================================== */

/**
 * @brief Inicializa a UART para monitoramento via Terminal.
 */
void uart_init(void) {
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << TXEN0); 
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/**
 * @brief Envia caractere para a UART (compatível com printf).
 */
int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream);
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/* ==========================================================================
   CALLBACKS E UTILITÁRIOS
   ========================================================================== */

void my_delay_ms(uint32_t ms) {
    while (ms--) _delay_ms(1);
}

/**
 * @brief Callback que fornece os dados formatados para a biblioteca SD.
 */
uint16_t callback_escrita_rotacao(const uint8_t **data_ptr) {
    static uint8_t flag = 0;
    if (flag) { flag = 0; return 0; }
    *data_ptr = (const uint8_t *)log_buffer;
    flag = 1;
    return (uint16_t)strlen(log_buffer);
}

/* ==========================================================================
   LÓGICA DE GERENCIAMENTO DE ARQUIVOS (ROTAÇÃO)
   ========================================================================== */

/**
 * @brief Localiza ou cria o volume de log adequado no cartão.
 * 
 * Percorre o diretório raiz buscando por LOG001.TXT até LOG099.TXT.
 * Seleciona o primeiro arquivo que ainda possua espaço ou cria um novo volume.
 */
void gerenciar_proximo_volume(void) {
    SD_FileName file;
    uint32_t tamanho_atual = 0;

    for (uint8_t i = 1; i <= MAX_VOLUMES; i++) {
        sprintf(nome_arquivo_ativo, "LOG%03u.TXT", i);
        sd_set_filename(&file, nome_arquivo_ativo);

        // Se o arquivo não existir, cria e seleciona ele
        if (sd_file_exists(&sd_ctx, &fat_fs, &file) != SD_SUCCESS) {
            sd_file_create(&sd_ctx, &fat_fs, &file);
            printf("[SISTEMA] Novo volume de log criado: %s\n", nome_arquivo_ativo);
            return;
        }

        // Se existir, verifica se ainda há espaço disponível (Limite definido em TAMANHO_MAX_BYTES)
        sd_file_get_size(&sd_ctx, &fat_fs, &file, &tamanho_atual);
        if (tamanho_atual < TAMANHO_MAX_BYTES) {
            printf("[SISTEMA] Retomando volume: %s (%lu bytes)\n", nome_arquivo_ativo, tamanho_atual);
            return;
        }
    }
    printf("[ERRO] Memoria cheia ou limite de volumes atingido!\n");
}

/**
 * @brief Formata e grava um registro CSV com data, hora e 4 sensores.
 * 
 * Caso o arquivo atual exceda o limite de tamanho, a rotação é acionada.
 */
void registrar_dado_com_rotacao(int s1, int s2, int s3, int s4) {
    SD_FileName file;
    uint32_t tamanho = 0;
    Timestamp agora = {30, 03, 26, 20, 15, 00}; // Dados fixos para fins didáticos

    sd_set_filename(&file, nome_arquivo_ativo);
    sd_file_get_size(&sd_ctx, &fat_fs, &file, &tamanho);

    // Verifica se precisa rotacionar para o próximo arquivo antes da nova escrita
    if (tamanho >= TAMANHO_MAX_BYTES) {
        printf("[ALERTA] Volume %s cheio! Rotacionando...\n", nome_arquivo_ativo);
        gerenciar_proximo_volume();
        sd_set_filename(&file, nome_arquivo_ativo);
    }

    // Formatação da linha no padrão CSV (Ponto e vrgula para Excel PT-BR)
    snprintf(log_buffer, sizeof(log_buffer), 
             "%02u/%02u/%02u;%02u:%02u:%02u;%d;%d;%d;%d\n",
             agora.dia, agora.mes, agora.ano, agora.hora, agora.min, agora.seg,
             s1, s2, s3, s4);

    // Gravação via Streaming (Append)
    if (sd_file_append_stream(&sd_ctx, &fat_fs, &file, callback_escrita_rotacao) == SD_SUCCESS) {
        printf("Gravado [%s]: %s", nome_arquivo_ativo, log_buffer);
    } else {
        printf("[ERRO] Falha ao gravar no SD!\n");
    }
}

/* ==========================================================================
   FUNÇÃO PRINCIPAL E LOOP DE TESTE
   ========================================================================== */

/**
 * @brief Ponto de entrada do programa.
 */
int main(void) {
    // 1. Inicializações de Sistema
    uart_init();
    stdout = &mystdout;
    _delay_ms(1000);

    printf("\n=== SISTEMA DE DATALOGGER COM ROTACAO (BACKUP) ===\n");
    printf("Canal Ciencia Eletrica - Tiago Henrique\n\n");

    // 2. Inicialização do Cartão SD e Partição FAT32
    if (sd_begin(&sd_ctx, my_delay_ms) != SD_SUCCESS) {
        printf("[ERRO] Cartao SD nao encontrado!\n");
        while(1);
    }

    if (sd_fat_mount(&sd_ctx, &fat_fs) != SD_SUCCESS) {
        printf("[ERRO] Falha ao montar particao FAT32!\n");
        while(1);
    }

    printf("[OK] Hardware pronto!\n");

    // 3. Gerencia o arquivo inicial (Busca o último ou cria o LOG001.TXT)
    gerenciar_proximo_volume();

    // 4. Loop de Simulação de Medidas
    for (uint16_t i = 1; i <= 150; i++) {
        // Simulação de valores de sensores
        int v1 = rand() % 100;
        int v2 = rand() % 100;
        int v3 = rand() % 500;
        int v4 = rand() % 1000;

        registrar_dado_com_rotacao(v1, v2, v3, v4);
        
        _delay_ms(100); // Intervalo entre registros
    }

    printf("\n=== TESTE FINALIZADO ===\n");
    printf("Verifique os arquivos LOGXXX.TXT no cartao.\n");

    while (1) {
        // Fim da execução
    }

    return 0;
}
