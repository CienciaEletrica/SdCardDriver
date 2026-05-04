/**
 * @example exemplo_2.c
 * Sistema de Datalogger CSV com busca por coluna para microcontroladores AVR.
 *
 * Este exemplo demonstra como criar um arquivo CSV, gravar múltiplos registros 
 * simulando sensores e realizar buscas inteligentes por valores em colunas específicas,
 * tudo otimizado para o compilador XC8 e baixa utilização de memória RAM.
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
#include <stdlib.h>
#include "sdcard.h"

/* ==========================================================================
   VARIÁVEIS GLOBAIS E ESTRUTURAS
   ========================================================================== */

/** @brief Contexto de hardware do SD Card */
SD_Context   sd_ctx;

/** @brief Informações do sistema de arquivos FAT32 montado */
FAT32_Info   fat_fs;

/** @brief Valor numérico que o sistema tentará localizar no arquivo */
static uint32_t busca_valor_global;

/** @brief Índice da coluna (1 a 5) onde a busca será realizada */
static uint8_t  busca_coluna_global;

/* ==========================================================================
   COMUNICAÇÃO SERIAL (UART) - MONITORAMENTO
   ========================================================================== */

#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD - 1)

/**
 * @brief Inicializa o módulo UART0 do ATmega328P.
 * Configura 9600 bps, 8 bits de dados e 1 stop bit.
 */
void uart_init(void) {
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << TXEN0); // Habilita apenas transmissão
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-N-1
}

/**
 * @brief Envia um caractere via UART, compatível com a biblioteca stdio.h.
 */
int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream); // Adiciona Carriage Return ao New Line
    while (!(UCSR0A & (1 << UDRE0))); // Aguarda buffer estar vazio
    UDR0 = c;
    return 0;
}

/** @brief Stream de saída para habilitar o uso do printf() */
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/* ==========================================================================
   UTILITÁRIOS
   ========================================================================== */

/**
 * @brief Função de delay customizada compatível com os tipos de dados da biblioteca SD.
 */
void my_delay_ms(uint32_t ms) {
    while (ms--) _delay_ms(1);
}

/* ==========================================================================
   CALLBACKS (FUNÇÕES DE RETORNO)
   ========================================================================== */

/**
 * @brief Gera linhas formatadas em CSV para gravação via Streaming.
 * 
 * @param data_ptr Ponteiro que receberá o endereço da string gerada.
 * @return uint16_t Quantidade de caracteres (bytes) a serem gravados.
 * @note Retornar 0 sinaliza o fim da gravação para a biblioteca.
 */
uint16_t callback_datalogger(const uint8_t **data_ptr) {
    static uint16_t contador = 1;
    static char linha[64]; // Buffer estático para manter a string ativa durante a gravação

    // Define o limite de 30 registros
    if (contador > 30) {
        contador = 1; // Reseta para próxima execução
        return 0;
    }

    // Simulação de dados de sensores (Cálculos fictícios)
    uint16_t s1 = 20 + (contador % 15);
    uint16_t s2 = 40 + (contador % 20);
    uint16_t s3 = 500 + (contador * 10);
    uint16_t s4 = 300 + (contador % 30);

    // Formata a linha: Contador,S1,S2,S3,S4\r\n
    sprintf(linha, "%u,%u,%u,%u,%u\r\n", contador, s1, s2, s3, s4);

    *data_ptr = (const uint8_t *)linha;
    contador++;
    return (uint16_t)strlen(linha);
}

/**
 * @brief Processa blocos de dados lidos do SD em tempo real para busca de valores.
 * 
 * Esta função reconstrói as linhas do arquivo e usa tokenização (strtok) para
 * isolar colunas e comparar valores.
 * 
 * @param data Ponteiro para o bloco de dados lido (setor).
 * @param len Tamanho do bloco lido.
 */
void callback_busca(const uint8_t *data, uint16_t len) {
    static char buffer[128]; // Buffer para reconstruir a linha do CSV
    static uint8_t idx = 0;

    for (uint16_t i = 0; i < len; i++) {
        char c = (char)data[i];

        // Verifica fim de linha (\r ou \n)
        if (c == '\r' || c == '\n') {
            if (idx > 0) {
                buffer[idx] = '\0'; // Finaliza a string da linha

                // Cria uma cópia da linha pois o strtok modifica o original
                char copia_linha[128];
                strcpy(copia_linha, buffer);

                char *token = strtok(buffer, ",");
                uint8_t col = 1;

                while (token) {
                    // Verifica se chegamos na coluna desejada
                    if (col == busca_coluna_global) {
                        // Converte texto para número e compara
                        if ((uint32_t)atoi(token) == busca_valor_global) {
                            printf(">>> LINHA ENCONTRADA: %s\n", copia_linha);
                        }
                        break;
                    }
                    token = strtok(NULL, ",");
                    col++;
                }
                idx = 0; // Prepara para a próxima linha
            }
        } else if (idx < 127) {
            buffer[idx++] = c; // Armazena caractere no buffer de linha
        }
    }
}

/* ==========================================================================
   LÓGICA PRINCIPAL DO DATALOGGER
   ========================================================================== */

/**
 * @brief Inicia a busca por um valor em uma coluna específica do arquivo CSV.
 * 
 * @param valor_busca Número inteiro a ser procurado.
 * @param coluna Índice da coluna (1 = Primeira coluna, 2 = Segunda...).
 */
void busca_valor_coluna(uint32_t valor_busca, uint8_t coluna) {
    printf("\n[Busca] Procurando %lu na coluna %u...\n", valor_busca, coluna);

    busca_valor_global  = valor_busca;
    busca_coluna_global = coluna;

    SD_FileName arquivo;
    sd_set_filename(&arquivo, "DATALOG.CSV");

    // Lê o arquivo em modo streaming processando cada bloco via callback_busca
    SD_Status st = sd_file_read_stream(&sd_ctx, &fat_fs, &arquivo, callback_busca);
    if (st != SD_SUCCESS && st != SD_ERR_NOT_FOUND) {
        printf("Erro na leitura: %02X\n", st);
    }
}

/**
 * @brief Executa o teste completo: Inicialização, Gravação e Busca.
 */
void teste_datalogger_csv(void) {
    SD_Status status;
    SD_FileName arquivo;

    printf("\n=== DATALOGGER CSV + BUSCA POR COLUNA ===\n");
    printf("Canal Ciencia Eletrica - Tiago Henrique\n\n");

    // 1. Inicializa o hardware SPI e o Cartão SD
    status = sd_begin(&sd_ctx, my_delay_ms);
    if (status != SD_SUCCESS) { printf("ERRO Inicializacao: %02X\n", status); return; }
    printf("[OK] Cartao Detectado\n");

    // 2. Monta a partição FAT32 (Root-only)
    status = sd_fat_mount(&sd_ctx, &fat_fs);
    if (status != SD_SUCCESS) { printf("ERRO Particao: %02X\n", status); return; }
    printf("[OK] FAT32 Montada\n");

    sd_set_filename(&arquivo, "DATALOG.CSV");

    // 3. Limpa testes anteriores
    sd_file_delete(&sd_ctx, &fat_fs, &arquivo);
    printf("[OK] Arquivo anterior removido\n");

    // 4. Cria novo arquivo
    status = sd_file_create(&sd_ctx, &fat_fs, &arquivo);
    if (status != SD_SUCCESS) { printf("ERRO Criacao: %02X\n", status); return; }

    // 5. Gravação via Streaming (Gera 30 registros dinâmicos)
    printf("[OK] Gravando 30 registros...\n");
    status = sd_file_append_stream(&sd_ctx, &fat_fs, &arquivo, callback_datalogger);
    if (status != SD_SUCCESS) { printf("ERRO Gravacao: %02X\n", status); return; }

    // 6. Listagem para conferência
    printf("\n[Diretorio Atual]:\n");
    sd_dir_list(&sd_ctx, &fat_fs);

    // 7. Testes de Busca Inteligente
    printf("\n=== INICIANDO TESTES DE BUSCA ===\n");
    busca_valor_coluna(15, 1);   // Procura '15' na coluna 1 (Contador)
    busca_valor_coluna(678, 4);  // Procura '678' na coluna 4 (Sensor 3)
    busca_valor_coluna(25, 2);   // Procura '25' na coluna 2 (Sensor 1)

    printf("\n=== DATALOGGER FINALIZADO ===\n");
}

/* ==========================================================================
   ENTRADA DO PROGRAMA
   ========================================================================== */

/**
 * @brief Função principal.
 */
int main(void) {
    uart_init();
    stdout = &mystdout; // Redireciona printf para a UART
    
    _delay_ms(1000); // Estabilização

    teste_datalogger_csv();

    while (1) {
        // O processador permanece aqui após o teste
    }
}
