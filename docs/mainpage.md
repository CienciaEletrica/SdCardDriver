# Driver SD Card FAT32 para AVR {#mainpage}
\image html FIGURA2.png "Ciência Elétrica" width=70px

Bem-vindo à documentaçãoo oficial da biblioteca **AVR SD Card**. Este driver foi desenvolvido para permitir que microcontroladores da linha AVR (como o ATmega328P) manipulem arquivos em cartões SD formatados em **FAT32**.

## Principais Recursos
- **Baixo Consumo de RAM:** Utiliza buffers otimizados para operar em apenas 2KB de SRAM.
- **Streaming de Dados:** Funções de leitura e escrita via callback para processamento em tempo real.
- **Gerenciamento FAT32:** Suporte a criação, exclusão, append e expansão automática de diretórios.
- **Documentação Completa:** Grafos de chamadas gerados automaticamente para facilitar o entendimento do fluxo SPI/FAT.

## Pinagem Sugerida (ATmega328P)

| SD Card | AVR Pin | Função |
|:-------:|:-------:|:------:|
| CS      | PB2     | SS     |
| MOSI    | PB3     | MOSI   |
| MISO    | PB4     | MISO   |
| SCK     | PB5     | SCK    |

## Exemplos Completos de Estudo
Para facilitar o aprendizado, acesse a nossa seção dedicada:
- \subpage exemplos "Clique aqui para ver a lista de exemplos detalhada"

## Exemplo Rápido
```c
#define F_CPU 16000000UL
#include <util/delay.h>
#include "sdcard.h"

void my_delay_ms(uint32_t ms) {
    while (ms--) _delay_ms(1);
}

SD_Context sd_ctx;
FAT32_Info fat_fs;

void main() {
    sd_begin(&sd_ctx, my_delay_ms);
    if (sd_fat_mount(&sd_ctx, &fat_fs) == SD_SUCCESS) {
        sd_dir_list(&sd_ctx, &fat_fs);
    }
}
```
