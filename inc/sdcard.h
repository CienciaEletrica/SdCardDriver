/**
 * @file sdcard.h
 * @brief Driver SD Card SPI + FAT32 (Root-only) para AVR
 * @version V1.0
 * @author Tiago Henrique dos Santos / Canal do Youtube: Ciência Elétrica
 * @date 2026
 */

#ifndef SDCARD_H
#define SDCARD_H

/* Definição automática de Clock */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* Includes de Sistema centralizados */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>


/* ================================================================
   Definições de I/Os
 ================================================================== */
/**
 * @brief Define as I/Os usadas na comunicação SPI com o sdcard
 */
# define SS_PIN     (1 << PB2)
# define MOSI_PIN   (1 << PB3)
# define MISO_PIN   (1 << PB4)
# define SCK_PIN    (1 << PB5)
#define SD_DDR      DDRB
#define SD_PORT     PORTB

/* ================================================================
   CONFIGURAÇÃO PÚBLICA
   ================================================================ */

/** Velocidades do barramento SPI */
#define SD_SPI_SLOW 0   /**< ~400 kHz - usado na inicialização */
#define SD_SPI_FAST 1   /**< Velocidade máxima (fosc/2) */

/* ================================================================
   CÓDIGOS DE RETORNO
   ================================================================ */

typedef enum {
    SD_SUCCESS       = 0x00, /**< Operação concluída com sucesso */
    SD_ERR_TIMEOUT   = 0xFF, /**< Timeout na comunicação */

    /* Inicialização */
    SD_ERR_IDLE      = 0x01, /**< Falha ao entrar em modo Idle */
    SD_ERR_VOLTAGE   = 0x02, /**< Cartão no aceita a tensão */
    SD_ERR_INITIALIZATION = 0x03, /**< Falha na inicialização */
    SD_ERR_READY     = 0x04, /**< Cartão não ficou pronto */

    /* Sistema de arquivos */
    SD_ERR_MBR       = 0x10, /**< MBR inválido */
    SD_ERR_BOOT      = 0x11, /**< Boot sector inválido */
    SD_ERR_MOUNT     = 0x12, /**< Falha ao montar FAT32 */

    /* Arquivos */
    SD_ERR_NOT_FOUND = 0x20, /**< Arquivo não encontrado */
    SD_ERR_FILE_FULL = 0x21, /**< Diretório cheio */
    SD_ERR_DISK_FULL = 0x22, /**< Disco sem espaço */
    SD_ERR_WRITE     = 0x23  /**< Falha de escrita */
} SD_Status;

/* ================================================================
   TIPOS PÚBLICOS
   ================================================================ */
/**
 * @brief Callback para leitura de dados em streaming
 *
 * Esta função é chamada a cada setor lido.
 *
 * @param data Ponteiro para os dados lidos
 * @param len Quantidade de bytes válidos
 */
typedef void (*sd_stream_callback_t)(const uint8_t *data, uint16_t len);

/** Callback para fornecimento de dados em append (retorna 0 quando terminar) */
typedef uint16_t (*sd_stream_reader_t)(const uint8_t **data);

/**
 * @brief Callback para fornecimento de dados em escrita streaming
 *
 * Esta função deve preencher o buffer com dados a serem escritos.
 *
 * @param data Buffer onde os dados devem ser escritos
 * @param max_len Tamanho máximo disponivel no buffer
 * @return Número de bytes válidos escritos no buffer
 */
typedef uint16_t (*sd_write_callback_t)(uint8_t *data, uint16_t max_len);

typedef struct {
    uint8_t (*spi_tx_rx)(uint8_t data); /**< Transmite e recebe via SPI */
    void (*cs_low)(void);               /**< Ativa Chip Select */
    void (*cs_high)(void);              /**< Desativa Chip Select */
    void (*delay_ms)(uint32_t ms);      /**< Delay em milissegundos */
    void (*set_speed)(uint8_t speed_mode);/**< Seleção de clock 0=Lento, 1=Rápido */
} SD_HAL;

typedef struct {
    uint32_t fat_start;     /**< LBA inicial da partição */ //A retirar
    uint32_t fat_lba;           /**< LBA da tabela FAT */
    uint32_t data_start;        /**< Início da área de dados */
    uint32_t root_dir_lba;      /**< LBA do diretório raiz */
    uint32_t fsinfo_lba;        /**< LBA do setor FSInfo */
    uint8_t  sct_per_clus;      /**< Setores por cluster */
    uint32_t root_cluster;      /**< Cluster inicial do root */
    uint32_t fat_size;          /**< Tamanho da FAT em setores */
    uint8_t  num_fats;          /**< Número de FATs (geralmente 2) */
    uint32_t next_free_cluster; /**< Hint para próxima alocação */
    uint32_t fat_cache_lba;     /**< Cache do último setor FAT lido */
    uint32_t last_cluster;      /**< Último cluster usado (cache) */
    uint32_t last_dir_lba;      /**< Cache do último LBA de diretório com espaço */
    uint16_t last_dir_offset;   /**< Cache do último offset (0-480) usado */
} FAT32_Info;

/**
 * @brief Nome de arquivo no formato 8.3
 */
typedef struct {
    char name[8];   /**< Nome (8 caracteres, maiúsculo, preenchido com espaço) */
    char ext[3];    /**< Extensão (3 caracteres, maiúsculo, preenchido com espaço) */
} SD_FileName;

/**
 * @brief Contexto da biblioteca SD Card
 *
 * Armazena estado interno e interface de hardware.
 */
typedef struct {
    SD_HAL hal;            /**< Interface de hardware */
    uint8_t card_type;     /**< 0: desconhecido, 1: SDSC, 2: SDHC */
    uint8_t buffer[512];   /**< Buffer interno de setor */
} SD_Context;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} SD_DateTime;

// Declaração externa para ser vista pelo main.c e sdcard.c
extern SD_DateTime g_sd_datetime; 

/* ================================================================
   API PÚBLICA
   ================================================================ */
/**
 * @brief Prepara o contexto da biblioteca e inicializa a interface de hardware.
 * 
 * Configura os ponteiros de função HAL e executa a rotina inicial de 
 * sincronização do cartão SD (sd_init). Deve ser a primeira função chamada 
 * no fluxo do programa principal (main).
 * 
 * @param ctx         Ponteiro para o contexto que será inicializado.
 * @param delay_func  Ponteiro para a função de delay (ms) do sistema.
 * @return SD_Status SD_SUCCESS se o cartão foi reconhecido e inicializado.
 */
SD_Status sd_begin(SD_Context *ctx, void (*delay_func)(uint32_t ms));

/**
 * @brief Inicialização rápida para AVR (configura pinos + HAL interno)
 *
 * Configura automaticamente:
 * - Pinos PB2=CS, PB3=MOSI, PB5=SCK, PB4=MISO
 * - SPI Mestre, modo 0
 * - HAL interno (transfer, cs_low, cs_high)
 *
 * @note O delay_ms deve ser injetado antes ou via sd_begin()
 */
SD_Status sd_quick_init(SD_Context *ctx);

/**
 * @brief Localiza e monta o sistema de arquivos FAT32 no cartão SD.
 * 
 * Realiza a leitura do MBR para localizar a primeira partição primária. 
 * Em seguida, extrai os parâmetros do BPB (BIOS Parameter Block): 
 * setores por cluster, setores reservados, localização das FATs e do diretório raiz.
 * Inicializa também os caches internos de diretório e FAT para operações O(1).
 * 
 * @param ctx Ponteiro para o contexto da biblioteca SD inicializado.
 * @param fs Ponteiro para a estrutura FAT32_Info que será preenchida.
 * @return SD_Status SD_SUCCESS, SD_ERR_MBR (assinatura inválida) ou SD_ERR_BOOT.
 */
SD_Status sd_fat_mount(SD_Context *ctx, FAT32_Info *fs);

/**
 * @brief Converte uma string convencional para o formato FAT 8.3.
 * 
 * Limpa a estrutura de destino com espaços e realiza a conversão automática 
 * para caracteres maiúsculos. O nome é limitado a 8 caracteres e a extensão a 3.
 * Caracteres especiais e pontos extras são ignorados conforme o padrão FAT.
 * 
 * @param file_out Ponteiro para a estrutura SD_FileName que será preenchida.
 * @param name_in  String de entrada (ex: "LOG_2026.txt" vira "LOG_2026" e "TXT").
 * @return SD_Status SD_SUCCESS ou SD_ERR_READY se os ponteiros forem nulos.
 */
SD_Status sd_set_filename(SD_FileName *file_out, const char *name_in);

/**
 * @brief Verifica se arquivo existe
 *
 * @param ctx Contexto
 * @param fs Sistema FAT32
 * @param file Nome do arquivo
 *
 * @return SD_Status
 */
SD_Status sd_file_exists(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file);

/**
 * @brief Obtém tamanho do arquivo
 *
 * @param ctx Contexto
 * @param fs Sistema FAT32
 * @param file Nome do arquivo
 * @param size  Ponteiro para retornar o tamanho do arquivo em bytes
 *
 * @return SD_Status
 */
SD_Status sd_file_get_size(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file, uint32_t *size);

/**
 * @brief Lê o conteúdo de um arquivo FAT32 para um buffer externo
 *
 * Esta função realiza a leitura completa de um arquivo, percorrendo
 * toda a cadeia de clusters (cluster chain) na FAT32.
 *
 * Os dados são copiados sequencialmente para o buffer fornecido pelo usuário.
 *
 * @param ctx Ponteiro para o contexto da biblioteca SD
 * @param fs Ponteiro para a estrutura do sistema FAT32 montado
 * @param file Ponteiro para estrutura contendo nome do arquivo (formato 8.3)
 * @param buffer Buffer externo onde os dados sero armazenados
 * @param buffer_size Tamanho mximo do buffer em bytes
 *
 * @return
 * - SD_SUCCESS: leitura realizada com sucesso
 * - SD_ERR_NOT_FOUND: arquivo não encontrado
 * - SD_ERR_READ: erro durante leitura de bloco ou cluster
 * - SD_ERR_DISK_FULL: buffer insuficiente para armazenar o arquivo
 */
SD_Status sd_file_read(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file, uint8_t *buffer, uint32_t buffer_size);

/**
 * @brief Lê o conteúdo de um arquivo em modo streaming via callback.
 * 
 * Recomendado para AVR por economizar memória RAM, processando o arquivo setor a setor.
 * 
 * @param ctx Ponteiro para o contexto da biblioteca.
 * @param fs Ponteiro para a estrutura FAT32 montada.
 * @param file Ponteiro para a estrutura com o nome do arquivo (8.3).
 * @param callback Função chamada a cada 512 bytes lidos.
 * @return SD_Status Status da operação de leitura.
 */
SD_Status sd_file_read_stream(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file, sd_stream_callback_t callback);

/**
 * @brief Cria um novo arquivo no diretório raiz com expansão de cluster.
 * 
 * @param ctx       Ponteiro para o contexto da biblioteca.
 * @param fs        Ponteiro para a estrutura FAT32 montada.
 * @param file_name Ponteiro para a estrutura com o nome do arquivo.
 * @return SD_Status Status da operação.
 */
SD_Status sd_file_create(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file_name);

/**
 * @brief Escreve dados em um arquivo FAT32 existente (modo streaming)
 *
 * Sobrescreve o conteúdo do arquivo a partir do início, utilizando
 * a cadeia de clusters já existente.
 *
 * @note Não aloca novos clusters. Limitado ao tamanho atual do arquivo
 * 
 * @param ctx Contexto SD
 * @param fs Sistema FAT32 montado
 * @param file Nome do arquivo (8.3)
 * @param callback Função que fornece os dados a serem escritos
 *
 * @return
 * - SD_SUCCESS: escrita concluída
 * - SD_ERR_NOT_FOUND: arquivo não encontrado
 * - SD_ERR_WRITE: erro durante escrita
 */
SD_Status sd_file_write_stream(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file, sd_write_callback_t callback);

/**
 * @brief Adiciona dados ao final de um arquivo, expandindo clusters se necessário.
 * 
 * @param ctx Ponteiro para o contexto da biblioteca.
 * @param fs Ponteiro para a estrutura FAT32 montada.
 * @param file Ponteiro para a estrutura com o nome do arquivo (8.3).
 * @param reader Função que fornece os dados a serem gravados.
 * @return SD_Status SD_SUCCESS, SD_ERR_WRITE ou SD_ERR_DISK_FULL.
 */
SD_Status sd_file_append_stream(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file, sd_stream_reader_t reader);

/**
 * @brief Exclui um arquivo do diretório e libera seu espaço no disco.
 * 
 * @param ctx  Ponteiro para o contexto SD.
 * @param fs   Ponteiro para o FAT32 montado.
 * @param file Nome do arquivo (8.3).
 * @return SD_Status Status da exclusão.
 */
SD_Status sd_file_delete(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file);

/**
 * @brief Sincroniza o tamanho do arquivo no diretório físico (Flush).
 * 
 * Atualiza o campo de tamanho (4 bytes) na entrada de diretório correspondente.
 * Esta função deve ser chamada após operações de escrita para garantir que, 
 * em caso de queda de energia, os dados gravados não sejam perdidos pelo sistema.
 * 
 * @param ctx Ponteiro para o contexto da biblioteca SD.
 * @param fs Ponteiro para o sistema FAT32 montado.
 * @param file Estrutura com o nome do arquivo (8.3).
 * @param current_size O tamanho atualizado do arquivo em bytes.
 * @return SD_Status Status da gravação no setor de diretório.
 */
SD_Status sd_file_sync(SD_Context *ctx, FAT32_Info *fs, const SD_FileName *file, uint32_t current_size);

/**
 * @brief Lista todos os arquivos presentes no diretório raiz via Serial.
 * 
 * Percorre toda a corrente de clusters do diretório raiz e imprime o nome, 
 * extensão e tamanho de cada arquivo válido. Útil para depuração e verificação 
 * de persistência após operações de escrita/deleção.
 * 
 * @param ctx Ponteiro para o contexto da biblioteca.
 * @param fs  Ponteiro para a estrutura FAT32 montada.
 */
void sd_dir_list(SD_Context *ctx, FAT32_Info *fs);

/**
 * @brief Calcula a capacidade total do cartão SD em Megabytes (MB).
 * 
 * Realiza a leitura do registrador CSD (Card Specific Data) via CMD9. 
 * O cálculo é otimizado para a estrutura CSD v2.0, utilizada em cartões 
 * de alta capacidade (SDHC e SDXC).
 * 
 * @param ctx Ponteiro para o contexto da biblioteca SD.
 * @return uint32_t Capacidade em MB ou 0 em caso de erro de leitura ou versão incompatível.
 */
uint32_t sd_get_capacity_mb(SD_Context *ctx);

#endif // SDCARD_H
