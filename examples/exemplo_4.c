/**
 * @example exemplo_4.c
 * 
 * Exemplo de leitura de parâmetros de configuração (.ini) via SD Card.
 * 
 * Demonstra como carregar parâmetros do sistema (como taxa de amostragem) 
 * de um arquivo de texto no SD, permitindo configurar o hardware sem 
 * precisar recompilar o código. Inclui função de delay variável.
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
   DEFINIÇÃO DAS VARIÁVEIS DE CONTEXTO
   ========================================================================== */

/** @brief Instância real do contexto do SD (Alocação de memória) */
SD_Context sd_ctx; 

/** @brief Instância real da estrutura FAT32 (Alocação de memória) */
FAT32_Info fat_fs; 

/* ==========================================================================
   CONFIGURAÇÕES DA UART (9600 bps)
   ========================================================================== */
#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD - 1)

void uart_init(void) {
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << TXEN0); 
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream);
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/* ==========================================================================
   VARIÁVEIS GLOBAIS DE CONFIGURAÇÃO (LIDAS DO SD)
   ========================================================================== */

/** @brief Taxa de amostragem em ms lida do arquivo CONFIG.ini */
uint16_t cfg_sampling_rate = 1000; 

/** @brief Limite de temperatura lido do arquivo CONFIG.ini */
float cfg_temp_limit = 50.0;

/** @brief ID do dispositivo lido do arquivo CONFIG.ini */
uint8_t cfg_device_id = 1;

extern SD_Context sd_ctx;
extern FAT32_Info fat_fs;

/** @brief Buffer para processamento de cada linha do arquivo .ini */
static char line_buffer[64];

/* ==========================================================================
   FUNÇÕES DIDÁTICAS
   ========================================================================== */

/**
 * @brief Executa um delay variável em milissegundos.
 * 
 * Como a macro _delay_ms() do AVR exige um valor constante em tempo de 
 * compilação, usamos esta função que repete o delay de 1ms n vezes, 
 * permitindo usar o valor lido do cartão SD.
 * 
 * @param ms Quantidade de milissegundos para aguardar.
 */
void delay_ms_variavel(uint16_t ms) {
    while (ms--) {
        _delay_ms(1);
    }
}

/**
 * @brief Função de delay exigida pela biblioteca sd_begin.
 */
void my_delay_ms_lib(uint32_t ms) {
    while (ms--) _delay_ms(1);
}

/* ==========================================================================
   CALLBACK DO PARSER (PROCESSADOR DE TEXTO)
   ========================================================================== */

/**
 * @brief Callback chamado pela sd_file_read_stream para processar dados lidos.
 * 
 * Identifica o separador '=' e associa o valor à chave correspondente.
 */
void callback_parser_config(const uint8_t *data, uint16_t len) {
    static uint8_t idx = 0;

    for (uint16_t i = 0; i < len; i++) {
        char c = (char)data[i];

        // Processa ao encontrar fim de linha (\n ou \r)
        if (c == '\n' || c == '\r') {
            if (idx > 0) {
                line_buffer[idx] = '\0';
                
                // Busca o caractere '=' para separar Chave de Valor
                char *divisor = strchr(line_buffer, '=');
                if (divisor) {
                    *divisor = '\0'; // Divide a string em duas partes
                    char *chave = line_buffer;
                    char *valor = divisor + 1;

                    // Atribuição baseada no nome da chave (Case Sensitive)
                    if (strcmp(chave, "SAMPLING") == 0) {
                        cfg_sampling_rate = (uint16_t)atoi(valor);
                        printf("[INFO] SAMPLING definido para %u ms\n", cfg_sampling_rate);
                    } 
                    else if (strcmp(chave, "TEMP_MAX") == 0) {
                        cfg_temp_limit = atof(valor);
                        printf("[INFO] TEMP_MAX definido para %.2f C\n", cfg_temp_limit);
                    }
                    else if (strcmp(chave, "DEV_ID") == 0) {
                        cfg_device_id = (uint8_t)atoi(valor);
                        printf("[INFO] DEV_ID definido para %u\n", cfg_device_id);
                    }
                }
                idx = 0; // Limpa índice para a próxima linha
            }
        } else if (idx < 63) {
            line_buffer[idx++] = c;
        }
    }
}

/* ==========================================================================
   CARREGAMENTO DE ARQUIVO
   ========================================================================== */

/**
 * @brief Localiza e processa o arquivo CONFIG.ini no cartão SD.
 */
void carregar_parametros_boot(void) {
    SD_FileName arquivo_config;
    sd_set_filename(&arquivo_config, "CONFIG.INI");

    printf("\n[SISTEMA] Buscando CONFIG.INI no SD Card...\n");

    if (sd_file_exists(&sd_ctx, &fat_fs, &arquivo_config) == SD_SUCCESS) {
        // Inicia o streaming de leitura para processar o arquivo linha a linha
        sd_file_read_stream(&sd_ctx, &fat_fs, &arquivo_config, callback_parser_config);
        printf("[SISTEMA] Parametros carregados com sucesso.\n");
    } else {
        printf("[AVISO] CONFIG.INI nao encontrado! Usando valores padrao de fabrica.\n");
    }
}

/* ==========================================================================
   PROGRAMA PRINCIPAL
   ========================================================================== */

/**
 * @brief Função main demonstrando o fluxo completo de carregamento e uso.
 */
int main(void) {
    // 1. Inicializa Monitor Serial e stdout
    uart_init();
    stdout = &mystdout;
    _delay_ms(1000);

    printf("\n=== TESTE DE PARSER DE CONFIGURACAO (.INI) ===\n");
    printf("Canal Ciencia Eletrica - Tiago Henrique\n\n");

    // 2. Inicializa Hardware SD e FAT32
    if (sd_begin(&sd_ctx, my_delay_ms_lib) == SD_SUCCESS) {
        if (sd_fat_mount(&sd_ctx, &fat_fs) == SD_SUCCESS) {
            
            printf("\n--- Conteudo do Cartao ---\n");
            sd_dir_list(&sd_ctx, &fat_fs); // Lista todos os arquivos reais no terminal
            
            // 3. Carrega as configurações dinâmicas do SD para a RAM
            carregar_parametros_boot();

            // 4. Demonstração do uso real da variável cfg_sampling_rate
            printf("\nIniciando medicao no ID %u...\n", cfg_device_id);
            printf("Aguardando %u ms entre leituras (valor do SD)\n\n", cfg_sampling_rate);

            for (uint8_t i = 1; i <= 5; i++) {
                printf("Leitura %u/5: Sensor OK (Limite Alarme: %.1f C)\n", i, cfg_temp_limit);
                
                // AQUI O DELAY VARIA CONFORME O ARQUIVO .ini DO SD
                delay_ms_variavel(cfg_sampling_rate); 
            }

        } else {
            printf("[ERRO] Falha ao montar particao FAT32!\n");
        }
    } else {
        printf("[ERRO] Cartao SD nao detectado!\n");
    }

    printf("\n=== TESTE FINALIZADO ===\n");
    while (1);
    return 0;
}
