/**
 * @example exemplo_1.c
 * EXEMPLO DIDÁTICO 1 da biblioteca SD Card SPI + FAT32
 * 
 * @author Tiago Henrique dos Santos / Canal Ciência Elétrica
 * @date 30 de Maro de 2026
 *
 * Este um exemplo simples e completo para quem est comeando a usar a lib:
 * 1. Inicializa o SD Card (sd_begin)
 * 2. Monta o FAT32 (sd_fat_mount)
 * 3. Cria o arquivo "EXEMPLO.TXT" no diretório raiz
 * 4. Escreve uma string de texto usando APPEND STREAMING (melhor método para AVR)
 * 5. Faz o flush (sd_file_sync) para garantir que o tamanho fique gravado
 * 6. Mostra o diretório raiz e a capacidade do cartão
 *
 * Tudo está dentro de uma única função: teste_basico_escrever_string()
 * O main() apenas chama essa função e entra em loop infinito.
 *
 * UART configurada em 9600 bps (monitor serial).
 * F_CPU = 16 MHz (padrão AVR).
 *
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>    // para salvar a string longa na Flash, não na RAM
#include "sdcard.h"

// =============================================
// VARIÁVEIS GLOBAIS DA BIBLIOTECA (obrigatórias)
// =============================================
SD_Context   sd_ctx;     // Contexto completo (HAL + buffer interno de 512 bytes)
FAT32_Info   fat_fs;     // Informações do sistema FAT32 montado

// =============================================
// UART 9600 bps (para monitor serial no PC)
// =============================================
#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD - 1)

void uart_init(void) {
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << TXEN0);                    // Apenas TX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8N1
}

int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream);
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

// =============================================
// DELAY (obrigatório para sd_begin)
// =============================================
void my_delay_ms(uint32_t ms) {
    while (ms--) {
        _delay_ms(1);
    }
}

// =============================================
// CALLBACK PARA APPEND STREAMING (sd_stream_reader_t)
// =============================================
/**
 * @brief Callback chamado pela biblioteca sd_file_append_stream().
 * 
 * A biblioteca chama esta função repetidamente até que ela retorne 0.
 * Aqui enviamos uma única string (pode ser bem maior se quiser).
 * 
 * @param data_ptr Ponteiro que deve apontar para os dados a serem escritos
 * @return Número de bytes válidos (0 = fim da transmissão)
 */
uint16_t callback_escreve_string(const uint8_t **data_ptr) {
    // String que será gravada no arquivo EXEMPLO.TXT
    static const char texto[] PROGMEM = 
        "Olá, mundo!\r\n"
        "Este arquivo foi criado pela biblioteca SD Card V19\r\n"
        "do Canal Ciência Elétrica (Tiago Henrique).\r\n"
        "Data da biblioteca: 30/03/2026\r\n"
        "Teste básico de escrita com append_stream.\r\n"
        "Fim do arquivo.\r\n";

    static uint8_t ja_enviado = 0;

    if (ja_enviado == 0) {
        *data_ptr = (const uint8_t *)texto;   // Aponta para a string
        ja_enviado = 1;
        return strlen_P(texto);               // Quantidade de bytes
    }

    ja_enviado = 0;
    return 0;   // Fim dos dados
}

// =============================================
// FUNÇÃO PRINCIPAL DO EXEMPLO
// =============================================
/**
 * @brief Demonstração completa e básica da biblioteca.
 * 
 * Todos os passos necessários para criar e escrever em um arquivo .txt
 * estão aqui, com explicação linha a linha.
 */
void teste_basico_escrever_string(void) {
    SD_Status status;
    SD_FileName arquivo;

    printf("\n=== EXEMPLO 1 ===\n");
    printf("Canal Ciência Elétrica - Tiago Henrique\n\n");

    // --------------------------------------------------------
    // 1. INICIALIZAÇÃO DO SD CARD (método recomendado)
    // --------------------------------------------------------
    printf("[1] sd_begin()... ");
    status = sd_begin(&sd_ctx, my_delay_ms);
    if (status != SD_SUCCESS) {
        printf("ERRO %02X\n", status);
        return;
    }
    printf("OK (SPI + cartão inicializado)\n");

    // --------------------------------------------------------
    // 2. MONTA O SISTEMA DE ARQUIVOS FAT32
    // --------------------------------------------------------
    printf("[2] sd_fat_mount()... ");
    status = sd_fat_mount(&sd_ctx, &fat_fs);
    if (status != SD_SUCCESS) {
        printf("ERRO %02X\n", status);
        return;
    }
    printf("OK (FAT32 montado com sucesso)\n");

    // --------------------------------------------------------
    // 3. PREPARA NOME DO ARQUIVO (formato 8.3)
    // --------------------------------------------------------
    printf("[3] sd_set_filename()... ");
    status = sd_set_filename(&arquivo, "EXEMPLO.TXT");
    if (status != SD_SUCCESS) {
        printf("ERRO nome invalido\n");
        return;
    }
    printf("OK (EXEMPLO.TXT)\n");

    // --------------------------------------------------------
    // 4. REMOVE ARQUIVO ANTIGO (opcional - para teste limpo)
    // --------------------------------------------------------
    printf("[4] sd_file_delete() (se existir)... ");
    sd_file_delete(&sd_ctx, &fat_fs, &arquivo);   // Ignora erro se não existir
    printf("OK\n");

    // --------------------------------------------------------
    // 5. CRIA O ARQUIVO VAZIO NO DIRETÓRIO RAIZ
    // --------------------------------------------------------
    printf("[5] sd_file_create()... ");
    status = sd_file_create(&sd_ctx, &fat_fs, &arquivo);
    if (status != SD_SUCCESS) {
        printf("ERRO %02X\n", status);
        return;
    }
    printf("OK\n");

    // --------------------------------------------------------
    // 6. ESCREVE A STRING USANDO APPEND STREAMING
    // --------------------------------------------------------
    printf("[6] sd_file_append_stream()... ");
    status = sd_file_append_stream(&sd_ctx, &fat_fs, &arquivo, callback_escreve_string);
    if (status != SD_SUCCESS) {
        printf("ERRO %02X\n", status);
        return;
    }
    printf("OK\n");

    // --------------------------------------------------------
    // 7. SINCRONIZA (FLUSH) - ESSENCIAL!
    //    Atualiza o tamanho do arquivo no diretório
    // --------------------------------------------------------
    uint32_t tamanho = strlen_P(
        "Olá, mundo!\r\n"
        "Este arquivo foi criado pela biblioteca SD Card\r\n"
        "do Canal Ciência Elétrica (Tiago Henrique).\r\n"
        "Data da biblioteca: 30/03/2026\r\n"
        "Teste básico de escrita com append_stream.\r\n"
        "Fim do arquivo.\r\n"
    );

    printf("[7] sd_file_sync() (flush)... ");
    status = sd_file_sync(&sd_ctx, &fat_fs, &arquivo, tamanho);
    if (status != SD_SUCCESS) {
        printf("ERRO %02X\n", status);
    } else {
        printf("OK (tamanho %lu bytes gravado)\n", tamanho);
    }

    // --------------------------------------------------------
    // 8. LISTAGEM FINAL E INFORMAÇÕES ÚTEIS
    // --------------------------------------------------------
    printf("\n[8] Listagem do diretório raiz:\n");
    sd_dir_list(&sd_ctx, &fat_fs);

    uint32_t capacidade_mb = sd_get_capacity_mb(&sd_ctx);
    printf("\nCapacidade do cartão: %lu MB\n", capacidade_mb);
    printf("\n=== ARQUIVO EXEMPLO.TXT CRIADO COM SUCESSO! ===\n");
    printf("Insira o cartão no PC e abra EXEMPLO.TXT\n\n");
}

// =============================================
// MAIN
// =============================================
int main(void) {
    // Configura UART para 9600 bps
    uart_init();
    stdout = &mystdout; // Faz printf() ir para o monitor serial

    _delay_ms(1000); // Tempo para o terminal abrir

    // Chama a função de exemplo (tudo acontece aqui)
    teste_basico_escrever_string();

    // Loop infinito (programa termina aqui)
    while (1) {
        // Se quiser piscar LED ou fazer outra coisa, coloque aqui
    }
}
