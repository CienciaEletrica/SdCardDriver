# 💾 Driver SD Card FAT32 para AVR (ATmega328P)

[![License: MIT](https://shields.io)](https://opensource.org)
[![AVR](https://shields.io)](https://microchip.com)
[![Compiler: XC8](https://shields.io)](https://microchip.com)

Biblioteca otimizada para manipulação de cartões SD formatados em **FAT32** utilizando microcontroladores AVR. Desenvolvida com foco em baixo consumo de memória RAM e confiabilidade para sistemas de telemetria e dataloggers.

---

## 🚀 Diferenciais Técnicos
- **Streaming Real:** Processamento de leitura e escrita via *callbacks*, permitindo manipular arquivos de Gigabytes com apenas 2KB de RAM.
- **Escrita Inteligente:** Motor de *append* corrigido para evitar "lixo" de memória (zero-padding), garantindo arquivos CSV perfeitamente compatíveis com Excel.
- **Gestão de Clusters:** Alocação dinâmica de setores e suporte a expansão automática de arquivos.
- **Manutenção de Disco:** Funções integradas para listagem de diretório (`ls`), exclusão (`rm`) e verificação de capacidade (`df`).

---

## 🔌 Conexão de Hardware (Pinagem)

Para o **ATmega328P** (Arduino Uno/Nano), utilize as seguintes conexões SPI:


| SD Card | AVR Pin | Função | Descrição |
|:-------:|:-------:|:------:|:---------|
| **VCC** | 3.3V    | Power  | Alimentação (⚠️ Nunca use 5V direto no SD) |
| **GND** | GND     | Ground | Terra Comum |
| **CS**  | PB2     | SS     | Chip Select (Ativo em nível baixo) |
| **MOSI**| PB3     | MOSI   | Master Out Slave In |
| **MISO**| PB4     | MISO   | Master In Slave Out |
| **SCK** | PB5     | SCK    | Serial Clock |

---

## 📂 Exemplos Inclusos

O repositório conta com 4 demonstrações completas:

1.  **[Datalogger CSV](examples/exemplo_datalogger_csv.c):** Gravação de 5 colunas de dados com busca inteligente por valores.
2.  **[Backup & Rotação](examples/exemplo_rotacao_logs.c):** Gerenciamento automático de volumes (LOG001.TXT, LOG002.TXT) com base no tamanho.
3.  **[Parser de Configuração](examples/exemplo_config_parser.c):** Carregamento de parâmetros do sistema via arquivo `CONFIG.INI`.
4.  **[Shell Serial](examples/exemplo_shell_sd.c):** Terminal interativo para gerenciar o cartão em tempo real.

---

## 🛠️ Como Usar

1. Adicione as pastas `/inc` e `/src` ao seu projeto no **MPLAB X**.
2. Configure a frequência do cristal (`F_CPU`) nas propriedades do projeto.
3. Inicialize o driver no seu `main.c`:
