# Exemplos de Uso {#exemplos}
Nesta seção, você encontrará exemplos práticos de aplicação da biblioteca **AVR SD Card**.

## 1. Testes Básicos de Primitivas
Este é um exemplo simples e completo para quem está começando a usar a lib.
- **Arquivo:** \example exemplo_1.c
- **Funcionalidades:** `sd_begin`, `sd_fat_mount`, `sd_set_filename`, `sd_file_create`, `sd_file_delete`, `sd_file_append_stream`, `sd_file_sync`, `sd_get_capacity_mb`.

## 2. Datalogger CSV com Busca por Coluna
Este exemplo demonstra como criar um arquivo, gravar dados de sensores e realizar buscas inteligentes.
- **Arquivo:** \example exemplo_2.c
- **Funcionalidades:** `sd_file_append_stream`, `sd_file_read_stream`, `strtok`.

## 3. Sistema de Datalogger com Rotação Automática de Arquivos e Timestamp
Este exemplo demonstra como gerenciar o armazenamento de forma profissional em cartões SD.
O sistema monitora o tamanho do arquivo atual e, ao atingir um limite (2,5KB), cria automaticamente um novo volume (LOG001.TXT, LOG002.TXT, etc).
Inclui carimbo de data/hora (Timestamp) simulado para cada registro.
- **Arquivo:** \example exemplo_3.c
- **Funcionalidades:** `sd_file_exists`, `sd_file_get_size`, `sd_file_create`, `sd_file_append_stream`.

## 4. Exemplo de leitura de parâmetros de configuração (.ini) via SD Card
Demonstra como carregar parâmetros do sistema (como taxa de amostragem) de um arquivo de texto no SD, permitindo configurar o hardware sem precisar recompilar o código. Inclui função de delay variável.
- **Arquivo:** \example exemplo_4.c
- **Funcionalidades:** `sd_file_read_stream`, `sd_set_filename`.
- O arquivo CONFIG.ini no SD deve seguir este formato (sem espaços)
- O arquivo CONFIG.ini desse exemplo está na pasta "examples"
- Deixe sempre uma linha vazia no final do arquivo CONFIG.ini

## 5. Terminal de Comandos (Shell) para diagnóstico e manutenção do SD Card
Este exemplo demonstra como interagir com o sistema de arquivos em tempo real via comandos seriais (UART), permitindo listar arquivos, verificar o espaço em disco e deletar logs antigos.
- **Arquivo:** \example exemplo_5.c
- **Funcionalidades:** `sd_dir_list`, `sd_get_capacity_mb`, `sd_file_delete`, `sd_set_filename`.

---
*Para ver o código-fonte completo, clique nos links dos arquivos acima ou acesse a aba **Exemplos** no menu superior.*
