// //-------------------------------------------Bibliotecas-------------------------------------------
// #include <stdio.h>   // Biblioteca padrão de entrada e saída
// #include <string.h>  // Biblioteca padrão de manipulação de strings
// #include <ctype.h>   // Biblioteca padrão de manipulação de caracteres
// #include <stdbool.h> // Biblioteca padrão de tipos booleanos
// #include <stdlib.h>  // Biblioteca padrão de alocação de memória e conversões
// #include <time.h>    // Biblioteca padrão de manipulação de tempo

#include "pico/stdlib.h"      // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "pico/unique_id.h"   // Biblioteca com recursos para trabalhar com os pinos GPIO do Raspberry Pi Pico
#include "pico/bootrom.h"     // Biblioteca com recursos para trabalhar com o bootrom da Raspberry Pi Pico
#include "pico/binary_info.h" // Biblioteca para informações binárias do Raspberry Pi Pico

#include "ssd1306.h" // Biblioteca para o display OLED SSD1306
#include "font.h"    // Biblioteca de fontes para o display OLED

#include "hardware/gpio.h" // Biblioteca de hardware de GPIO
#include "hardware/i2c.h"  // Biblioteca de hardware de I2C
#include "hardware/rtc.h"  // Biblioteca de hardware de RTC (Real Time Clock)
#include "hardware/pwm.h"  // Biblioteca de hardware de PWM

#include "ff.h"        // Biblioteca de sistema de arquivos FatFs
#include "diskio.h"    // Biblioteca de interface de disco
#include "f_util.h"    // Biblioteca de utilitários FatFs
#include "hw_config.h" // Biblioteca de configuração de hardware
#include "my_debug.h"  // Biblioteca de depuração personalizada
#include "rtc.h"       // Biblioteca de RTC
#include "sd_card.h"   // Biblioteca de cartão SD
#include "gy33.h"      // Biblioteca do sensor GY-33

//-------------------------------------------Definições-------------------------------------------
#define I2C_PORT i2c0 // Porta I2C para sensor gy-33
#define I2C_SDA 0     // Pino SDA para sensor gy-33
#define I2C_SCL 1     // Pino SCL para sensor gy-33

#define I2C_PORT_DISP i2c1 // Porta I2C para display
#define I2C_SDA_DISP 14    // Pino SDA para display
#define I2C_SCL_DISP 15    // Pino SCL para display

#define ENDERECO_DISP 0x3C // Endereço I2C do display SSD1306
#define DISP_W 128         // Largura do display SSD1306
#define DISP_H 64          // Altura do display SSD1306

#define BOTAO_A_PIN 5 // Pino do botão A
#define BOTAO_B_PIN 6 // Pino do botão B

#define LED_PIN_GREEN 11 // Pino do LED verde
#define LED_PIN_BLUE 12  // Pino do LED azul
#define LED_PIN_RED 13   // Pino do LED vermelho
#define BUZZER_A 21      // Pino do buzzer A
#define BUZZER_B 10      // Pino do buzzer B

//-------------------------------------------Variáveis Globais-------------------------------------------
static int addr = 0x74; // Endereço I2C do gy-33
ssd1306_t ssd;          // Estrutura para o display SSD1306

// Variáveis Globais para controle de estado do display
volatile int estado_display = 0;           // 0 = RGB, 1 = Normalizado, 2 = Lux
volatile uint32_t ultimo_tempo_clique = 0; // Debounce dos botões

bool captura_dados = false;         // Variável para controle de captura de dados
bool montar_sd = false;             // Variável para controle de montagem do SD
static bool gravacao_ativa = false; // Controla se a gravação está ativa

// Variáveis para controle de tempo
static absolute_time_t next_log_time;
volatile static absolute_time_t last_time = 0;
static absolute_time_t last_buzzer_time = 0;

// Nome do arquivo para gravação contínua
static char filename[30] = "test.txt";

static FIL arquivo_dados;         // Arquivo para gravação contínua
static int contador_amostras = 0; // Contador de amostras gravadas

//-------------------------------------------Prototipos de Funções-------------------------------------------
void gpio_irq_handler(uint gpio, uint32_t events);        // Função de tratamento de interrupção de GPIO
void setup();                                             // Função de configuração inicial
static sd_card_t *sd_get_by_name(const char *const name); // Função para obter o cartão SD pelo nome
static FATFS *sd_get_fs_by_name(const char *name);        // Função para obter o sistema de arquivos do cartão SD pelo nome
static void run_mount();                                  // Função para montar o cartão SD
static void run_unmount();                                // Função para desmontar o cartão SD
static void start_continuous_capture();                   // Função para iniciar a captura contínua
static void stop_continuous_capture();                    // Função para parar a captura contínua
static void process_continuous_capture();                 // Função para processar a captura contínua

//-------------------------------------------Função Principal-------------------------------------------
int main()
{
    stdio_init_all(); // Inicializa a biblioteca padrão de entrada e saída
    sleep_ms(2000);   // Aguarda 2 segundos para estabilização

    setup(); // Chama a função de configuração inicial

    // Inicializa o sensor de cor GY-33
    gy33_init(I2C_PORT);

    // Prepara o display SSD1306
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Aguardando...", 0, 0);
    ssd1306_send_data(&ssd);

    // Configura os LEDs iniciais em amarelo
    gpio_put(LED_PIN_RED, 1);
    gpio_put(LED_PIN_GREEN, 1);
    gpio_put(LED_PIN_BLUE, 0);

    // Loop principal
    while (true)
    {
        // Processa captura contínua se estiver ativa
        if (gravacao_ativa)
        {
            process_continuous_capture();
        }

        // Verifica se deve iniciar ou parar a captura
        if (captura_dados && !gravacao_ativa)
        {
            // Inicia a gravação
            start_continuous_capture();
        }
        else if (!captura_dados && gravacao_ativa)
        {
            // Para a gravação
            stop_continuous_capture();
        }

        // Verifica se deve montar ou desmontar o SD
        static bool sd_montado = false;
        if (montar_sd && !sd_montado)
        {
            // Monta o SD
            run_mount();
            sd_montado = true;
        }
        else if (!montar_sd && sd_montado)
        {
            // Desmonta o SD
            run_unmount();
            sd_montado = false;
        }

        sleep_ms(100); // Delay de 100ms
    }
    return 0;
}

//-------------------------------------------Funções-------------------------------------------
// Configuração inicial dos pinos GPIO
void setup()
{
    // Botões
    gpio_init(BOTAO_A_PIN);
    gpio_set_dir(BOTAO_A_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_A_PIN);

    gpio_init(BOTAO_B_PIN);
    gpio_set_dir(BOTAO_B_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_B_PIN);

    // LEDs
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    //     // Configuração do I2C para gy-33 sensor de cor
    i2c_init(I2C_PORT, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Configuração do I2C para o display SSD1306
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    ssd1306_init(&ssd, DISP_W, DISP_H, false, ENDERECO_DISP, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Exibe mensagem inicial no display
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Inicializando...", 0, 0);
    ssd1306_send_data(&ssd);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BOTAO_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

// Função para obter o cartão SD pelo nome
static sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

// Função para obter o sistema de arquivos do cartão SD pelo nome
static FATFS *sd_get_fs_by_name(const char *name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

// Função para montar o cartão SD
static void run_mount()
{
    printf("Tentando montar o cartão SD...\n");

    // LED amarelo (vermelho + verde) durante montagem
    gpio_put(LED_PIN_BLUE, 0);
    gpio_put(LED_PIN_RED, 1);
    gpio_put(LED_PIN_GREEN, 1);

    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        // Piscar roxo para indicar erro
        for (int i = 0; i < 3; i++)
        {
            gpio_put(LED_PIN_BLUE, 1);
            gpio_put(LED_PIN_RED, 1);
            gpio_put(LED_PIN_GREEN, 0);
            sleep_ms(200);
            gpio_put(LED_PIN_BLUE, 0);
            gpio_put(LED_PIN_RED, 0);
            sleep_ms(200);
        }
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Cartao SD Nao Detectado", 0, 0);
        ssd1306_send_data(&ssd);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);

        // Piscar roxo para indicar erro
        for (int i = 0; i < 3; i++)
        {
            gpio_put(LED_PIN_BLUE, 1);
            gpio_put(LED_PIN_RED, 1);
            gpio_put(LED_PIN_GREEN, 0);
            sleep_ms(200);
            gpio_put(LED_PIN_BLUE, 0);
            gpio_put(LED_PIN_RED, 0);
            sleep_ms(200);
        }
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Cartao SD Nao Detectado", 0, 0);
        ssd1306_send_data(&ssd);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Processo de montagem do SD ( %s ) concluído\n", pSD->pcName);

    // LED verde para sucesso / Sistema pronto
    gpio_put(LED_PIN_BLUE, 0);
    gpio_put(LED_PIN_RED, 0);
    gpio_put(LED_PIN_GREEN, 1);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Montagem", 0, 0);
    ssd1306_draw_string(&ssd, "Concluida...", 0, 10);
    ssd1306_draw_string(&ssd, "Aguardando...", 0, 20);
    ssd1306_send_data(&ssd);
}

// Função para desmontar o cartão SD
static void run_unmount()
{
    printf("Desmontando cartão SD...\n");

    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT; // in case medium is removed
    printf("SD ( %s ) desmontado\n", pSD->pcName);

    // LED amarelo para indicar desmontado
    gpio_put(LED_PIN_BLUE, 0);
    gpio_put(LED_PIN_RED, 1);
    gpio_put(LED_PIN_GREEN, 1);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Cartao SD", 0, 0);
    ssd1306_draw_string(&ssd, "Desmontado", 0, 10);
    ssd1306_send_data(&ssd);
}

// Função para iniciar a captura contínua
static void start_continuous_capture()
{
    if (gravacao_ativa)
    {
        printf("Gravação já está ativa!\n");
        return;
    }

    printf("\nIniciando gravação contínua do GY-33...\n");
    FRESULT res = f_open(&arquivo_dados, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        printf("\n[ERRO] Não foi possível abrir o arquivo para escrita. Monte o Cartao.\n");
        return;
    }

    // Escreve cabeçalho do arquivo CSV
    char header[] = "Amostra,Clear,Red,Green,Blue,cor\n";
    UINT bw;
    res = f_write(&arquivo_dados, header, strlen(header), &bw);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível escrever cabeçalho no arquivo.\n");
        f_close(&arquivo_dados);
        return;
    }

    gpio_put(LED_PIN_GREEN, 0);
    gpio_put(LED_PIN_BLUE, 0);
    gpio_put(LED_PIN_RED, 1);

    gravacao_ativa = true;
    contador_amostras = 0;
    printf("Gravação iniciada! Pressione o botão A novamente para parar.\n");
}

// Função para parar a captura contínua
static void stop_continuous_capture()
{
    if (!gravacao_ativa)
    {
        printf("Nenhuma gravação ativa para parar.\n");
        return;
    }

    gravacao_ativa = false;
    f_close(&arquivo_dados);
    printf("\nGravação interrompida! Total de amostras: %d\n", contador_amostras);
    printf("Dados salvos no arquivo %s.\n\n", filename);

    // Duplo beep para indicar fim da gravação

    sleep_ms(100);

    // Piscar azul para indicar gravação
    for (int i = 0; i < 3; i++)
    {
        gpio_put(LED_PIN_BLUE, 1);
        gpio_put(LED_PIN_RED, 1);
        gpio_put(LED_PIN_GREEN, 0);
        sleep_ms(200);
        gpio_put(LED_PIN_BLUE, 0);
        gpio_put(LED_PIN_RED, 0);
        sleep_ms(200);
    }

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Gravacao", 0, 0);
    ssd1306_draw_string(&ssd, "Interrompida...", 0, 10);
    ssd1306_draw_string(&ssd, "Dados Salvos", 0, 20);
    ssd1306_send_data(&ssd);

    // LED verde para indicar que a gravação foi parada / Sistema pronto
    gpio_put(LED_PIN_GREEN, 1);
}

// Função para processar uma amostra da captura contínua
static void process_continuous_capture()
{
    static absolute_time_t last_sample_time = 0;

    // Verifica se é hora de coletar uma nova amostra (100ms)
    if (absolute_time_diff_us(last_sample_time, get_absolute_time()) < 100000)
    {
        return;
    }
    last_sample_time = get_absolute_time();
    // Lê dados do SENSOR GY-33
    uint16_t r, g, b, c;
    gy33_read_color(I2C_PORT, &r, &g, &b, &c);
    const char *nome_da_cor = identificar_cor(r, g, b, c);
    printf("GY33 -> C:%u R:%u G:%u B:%u cor:%s\n", c, r, g, b, nome_da_cor);

    // Cria a string para salvar no cartão SD
    // Inclui o número da amostra e os 4 valores de cor
    char buffer[100]; // Ajustamos o tamanho do buffer, pois os dados são menores
    sprintf(buffer, "%d,%u,%u,%u,%u,%s\n",
            contador_amostras + 1,
            c, r, g, b, nome_da_cor);
    // Adiciona o nome da cor à string
    // strcat(buffer, nome_da_cor);
    // strcat(buffer, "\n");

    UINT bw;
    FRESULT res = f_write(&arquivo_dados, buffer, strlen(buffer), &bw);
    if (res != FR_OK)
    {
        printf("\n[ERRO] Falha na escrita. Interrompendo gravação.\n");
        stop_continuous_capture();
        return;
    }

    contador_amostras++;

    // LED vermelho para indicar captura de dados em andamento
    gpio_put(LED_PIN_GREEN, 0);
    gpio_put(LED_PIN_BLUE, 0);
    gpio_put(LED_PIN_RED, 1);

    // Força a escrita no cartão SD a cada 10 amostras
    if (contador_amostras % 10 == 0)
    {
        f_sync(&arquivo_dados);
        printf("Amostras coletadas: %d\n", contador_amostras);
    }
    // Atualiza o display SSD1306 com os valores de cor e o nome
    ssd1306_fill(&ssd, false); // Limpa a tela para a próxima atualização

    // Linha 0: Mensagem de status
    ssd1306_draw_string(&ssd, "Gravando Dados...", 0, 0);

    // Linha 1: Nome da cor (sempre visível)
    ssd1306_draw_string(&ssd, nome_da_cor, 0, 10);

    // Linha 2: Número da amostra
    char amostras_buffer[20];
    snprintf(amostras_buffer, sizeof(amostras_buffer), "Amostras: %d", contador_amostras);
    ssd1306_draw_string(&ssd, amostras_buffer, 0, 20);

    // Linha 3: Valores de Cor C e R
    char cr_buffer[20];
    snprintf(cr_buffer, sizeof(cr_buffer), "C: %u R: %u", c, r);
    ssd1306_draw_string(&ssd, cr_buffer, 0, 30);

    // Linha 4: Valores de Cor G e B
    char gb_buffer[20];
    snprintf(gb_buffer, sizeof(gb_buffer), "G: %u B: %u", g, b);
    ssd1306_draw_string(&ssd, gb_buffer, 0, 40);

    // Envia todos os dados para o display de uma vez
    ssd1306_send_data(&ssd);
}

// Função de tratamento de interrupção de GPIO
void gpio_irq_handler(uint gpio, uint32_t events)
{

    if (gpio == BOTAO_A_PIN || gpio == BOTAO_B_PIN)
    {
        if (absolute_time_diff_us(last_time, get_absolute_time()) < 500000) // 500ms debounce
        {
            return; // Ignora o evento se for muito rápido
        }
        last_time = get_absolute_time();
    }
    if (gpio == BOTAO_A_PIN)
    {
        if (events & GPIO_IRQ_EDGE_FALL)
        {
            captura_dados = !captura_dados; // Alterna entre iniciar/parar gravação
            printf("Botão A pressionado. Captura dados: %s\n", captura_dados ? "ATIVADA" : "DESATIVADA");
        }
    }
    else if (gpio == BOTAO_B_PIN)
    {
        if (events & GPIO_IRQ_EDGE_FALL)
        {
            montar_sd = !montar_sd; // Alterna entre montar/desmontar SD
            printf("Botão B pressionado. SD: %s\n", montar_sd ? "MONTAR" : "DESMONTAR");
        }
    }
}
// Função de interrupção dos botões
void tratar_interrupcao_gpio(uint gpio, uint32_t events)
{
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    if (tempo_atual - ultimo_tempo_clique < 250)
        return;
    ultimo_tempo_clique = tempo_atual;

    if (gpio == BOTAO_B_PIN)
    {
        estado_display = (estado_display + 1) % 3; // Próxima tela
    }
    else if (gpio == BOTAO_A_PIN)
    {
        estado_display = (estado_display - 1 + 3) % 3; // Tela anterior
    }
}
