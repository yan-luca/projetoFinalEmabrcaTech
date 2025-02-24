#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "hardware/gpio.h"

//----------------------------------PINOS---------------------------------
// Pinos do I2C
#define I2C_SDA 14
#define I2C_SCL 15
#define SDA_PIN 4
#define SCL_PIN 5

const uint DHT_PIN = 8;
const uint MAX_TIMINGS = 85;
#define TRIG_PIN 17 
#define ECHO_PIN 16
#define JOY_Y 26
#define JOY_X 27
#define BUTTON_A 5
#define BUTTON_B 6

#define LED_COUNT 25
#define LED_PIN 7


#define brightness 100
#define FONT_HEIGHT 8
#define LINE_SPACING (FONT_HEIGHT + 1)

#define NOTE_E7  2637
#define NOTE_G7  3136
#define NOTE_A7  3520
#define NOTE_F7  2794
#define NOTE_D7  2349
#define NOTE_B6  1975
#define NOTE_C7  2093

#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

#define BUZZER_PIN 10




int melody[] = {
    NOTE_E7, NOTE_G7, NOTE_A7, NOTE_F7, NOTE_D7, NOTE_B6,
    NOTE_C7, NOTE_D7, NOTE_E7, NOTE_G7, NOTE_A7, NOTE_F7,
    NOTE_D7, NOTE_E7, NOTE_C7, NOTE_A7, NOTE_B6, NOTE_C7
};
int melody2[] = {
    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_D5,
    NOTE_D5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_A5
};
int durations[] = {
    150, 150, 300, 300, 300, 150,
    300, 300, 150, 150, 300, 300,
    300, 150, 300, 300, 150, 300
};
int durations2[] = {
    300, 300, 300, 200, 200, 300, 300, 400,
    300, 300, 300, 200, 200, 300, 300, 500
};

void pwm_set_freq(uint slice_num, uint16_t freq) {
    pwm_set_wrap(slice_num, 125000000 / freq);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, (125000000 / freq) / 2);
}

void buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_freq(slice_num, 1000);  
    pwm_set_enabled(slice_num, true);
}

void play_note(int frequency, int duration) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    if (frequency == 0) {
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);  // Silêncio
    } else {
        pwm_set_freq(slice_num, frequency);
    }
    sleep_ms(duration);
}


void hc_sr04_init()
{
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
}

float hc_sr04_read_distance()
{
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);

    // Espera o sinal HIGH do ECHO
    while (gpio_get(ECHO_PIN) == 0);
    absolute_time_t start_time = get_absolute_time();

    // Espera o sinal LOW do ECHO
    while (gpio_get(ECHO_PIN) == 1);
    absolute_time_t end_time = get_absolute_time();

    // Calcula tempo de ida e volta do sinal (em microssegundos)
    int64_t pulse_time = absolute_time_diff_us(start_time, end_time);

    // Converte para distância (velocidade do som ≈ 343m/s ou 0.0343 cm/µs)
    float distance = (pulse_time * 0.0343) / 2.0;
    
}

//----------------------Funções Matriz de LEDs---------------------------
struct pixel_t
{
    uint8_t G, R, B; 
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; 

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

void npInit(uint pin)
{
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); 
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

void npWrite()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); 
}

void drawHeart()
{
    npClear();
    uint8_t heartColorR = brightness;
    uint8_t heartColorG = 0;
    uint8_t heartColorB = 0;

    npSetLED(6, heartColorR, heartColorG, heartColorB);
    npSetLED(14, heartColorR, heartColorG, heartColorB);
    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(17, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(8, heartColorR, heartColorG, heartColorB);
    npSetLED(10, heartColorR, heartColorG, heartColorB);
    npSetLED(2, heartColorR, heartColorG, heartColorB);

    npWrite();
}

void draw1()
{
    npClear();
    uint8_t heartColorR = 0;
    uint8_t heartColorG = 0;
    uint8_t heartColorB = brightness;

    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(22, heartColorR, heartColorG, heartColorB);
    npSetLED(17, heartColorR, heartColorG, heartColorB);
    npSetLED(12, heartColorR, heartColorG, heartColorB);
    npSetLED(2, heartColorR, heartColorG, heartColorB);
    npSetLED(3, heartColorR, heartColorG, heartColorB);
    npSetLED(1, heartColorR, heartColorG, heartColorB);
    npSetLED(7, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void draw2()
{
    npClear();
    uint8_t heartColorR = 0; 
    uint8_t heartColorG = 0;
    uint8_t heartColorB = brightness;

    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(22, heartColorR, heartColorG, heartColorB);
    npSetLED(21, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(12, heartColorR, heartColorG, heartColorB);
    npSetLED(6, heartColorR, heartColorG, heartColorB);
    npSetLED(3, heartColorR, heartColorG, heartColorB);
    npSetLED(2, heartColorR, heartColorG, heartColorB);
    npSetLED(1, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void draw3()
{
    npClear();
    uint8_t heartColorR = 0;
    uint8_t heartColorG = 0;
    uint8_t heartColorB = brightness;

    npSetLED(23, heartColorR, heartColorG, heartColorB);
    npSetLED(22, heartColorR, heartColorG, heartColorB);
    npSetLED(21, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(11, heartColorR, heartColorG, heartColorB);
    npSetLED(12, heartColorR, heartColorG, heartColorB);
    npSetLED(13, heartColorR, heartColorG, heartColorB);
    npSetLED(8, heartColorR, heartColorG, heartColorB);
    npSetLED(1, heartColorR, heartColorG, heartColorB);
    npSetLED(2, heartColorR, heartColorG, heartColorB);
    npSetLED(3, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void draw4()
{
    npClear();
    uint8_t heartColorR = 0; 
    uint8_t heartColorG = 0;
    uint8_t heartColorB = brightness;

    npSetLED(23, heartColorR, heartColorG, heartColorB);
    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(13, heartColorR, heartColorG, heartColorB);
    npSetLED(12, heartColorR, heartColorG, heartColorB);
    npSetLED(11, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(13, heartColorR, heartColorG, heartColorB);
    npSetLED(21, heartColorR, heartColorG, heartColorB);
    npSetLED(8, heartColorR, heartColorG, heartColorB);
    npSetLED(1, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void drawRostoFeliz()
{
    npClear();
    uint8_t heartColorR = 0; 
    uint8_t heartColorG = brightness;
    uint8_t heartColorB = 0;

    npSetLED(5, heartColorR, heartColorG, heartColorB);
    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(3, heartColorR, heartColorG, heartColorB);
    npSetLED(2, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(9, heartColorR, heartColorG, heartColorB);
    npSetLED(1, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void drawRostoBravo()
{
    npClear();
    uint8_t heartColorR = brightness; 
    uint8_t heartColorG = 0;
    uint8_t heartColorB = 0;

    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(5, heartColorR, heartColorG, heartColorB);
    npSetLED(6, heartColorR, heartColorG, heartColorB);
    npSetLED(7, heartColorR, heartColorG, heartColorB);
    npSetLED(8, heartColorR, heartColorG, heartColorB);
    npSetLED(9, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void drawRostoTriste()
{
    npClear();
    uint8_t heartColorR = 0; 
    uint8_t heartColorG = 0;
    uint8_t heartColorB = brightness;

    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(18, heartColorR, heartColorG, heartColorB);
    npSetLED(4, heartColorR, heartColorG, heartColorB);
    npSetLED(6, heartColorR, heartColorG, heartColorB);
    npSetLED(7, heartColorR, heartColorG, heartColorB);
    npSetLED(8, heartColorR, heartColorG, heartColorB);
    npSetLED(0, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void drawMusic(){
    npClear();
    uint8_t heartColorR = 0; 
    uint8_t heartColorG = 0;
    uint8_t heartColorB = brightness;

    npSetLED(5, heartColorR, heartColorG, heartColorB);
    npSetLED(13, heartColorR, heartColorG, heartColorB);
    npSetLED(16, heartColorR, heartColorG, heartColorB);
    npSetLED(23, heartColorR, heartColorG, heartColorB);
    npSetLED(22, heartColorR, heartColorG, heartColorB);
    npSetLED(21, heartColorR, heartColorG, heartColorB);
    npSetLED(20, heartColorR, heartColorG, heartColorB);
    npSetLED(19, heartColorR, heartColorG, heartColorB);
    npSetLED(10, heartColorR, heartColorG, heartColorB);
    npSetLED(8, heartColorR, heartColorG, heartColorB);
    npSetLED(9, heartColorR, heartColorG, heartColorB);
    npSetLED(6, heartColorR, heartColorG, heartColorB);
    npWrite();
}

void exibir_matriz_led(int opcao)
{
    npClear();
    switch (opcao)
    {
    case 0:
        draw1();
        break;
    case 1:
        draw2();
        break;
    case 2:
        draw3();
        break;
    case 3:
        draw4();
        break;
    default:
        break;
    }
}

void exibir_matriz_led_Emojis(int opcao)
{
    npClear();
    switch (opcao)
    {
    case 0:
        drawRostoFeliz();
        break;
    case 1:
        drawRostoTriste();
        break;
    case 2:
        drawRostoBravo();
        break;
    case 3:
        drawHeart();
        break;
    default:
        break;
    }
}

void exibir_matriz_led_Musica(int opcao)
{
    npClear();
    switch (opcao)
    {
    case 0:
    //printf("Tocando música...\n");
    drawMusic();
    for (int i = 0; i < 18; i++) {
        play_note(melody[i], durations[i]);
        sleep_ms(100);
    }
    pwm_set_chan_level(pwm_gpio_to_slice_num(BUZZER_PIN), PWM_CHAN_A, 0);
        break;
    case 1:
    for (int i = 0; i < 16; i++) {
        play_note(melody2[i], durations2[i]);
        sleep_ms(100);
    }
    pwm_set_chan_level(pwm_gpio_to_slice_num(BUZZER_PIN), PWM_CHAN_A, 0);
        break;
       
    default:
        break;
    }
}

void limparMatriz()
{
    npClear();
    npWrite();
}
//----------------------Funções Matriz de LEDs---------------------------

//----------------------Menu de Opções-----------------------------------
const char *menuOptions[] = {
    "Emojis",
    "Temperatura",
    "Distancia",
    "Musica"};

const char *menuEmojis[] = {
    "Feliz",
    "Triste",
    "Bravo",
    "Coracao"};

const char *menuMusica[] = {
    "Musica 1",
    "Musica 2"};

const int menuSize = sizeof(menuOptions) / sizeof(menuOptions[0]);
int menuIndex = 0;

void displayMenu(uint8_t *ssd, struct render_area *frame_area, int menuIndex)
{
    memset(ssd, 0, ssd1306_buffer_length); 

    for (int i = 0; i < menuSize; i++)
    {
        char line[20];
        if (i == menuIndex)
        {
            snprintf(line, sizeof(line), "> %s", menuOptions[i]); 
        }
        else
        {
            snprintf(line, sizeof(line), "%s", menuOptions[i]);
        }
        int y = 10 + (i * LINE_SPACING);
        ssd1306_draw_string(ssd, 5, y, line);
    }

    char line[20];
    snprintf(line, sizeof(line), "Opcao = %d", menuIndex + 1);
    ssd1306_draw_string(ssd, 0, 56, line);

    render_on_display(ssd, frame_area);
}

void displayMenuEmojis(uint8_t *ssd, struct render_area *frame_area, int menuIndex)
{
    memset(ssd, 0, ssd1306_buffer_length); // Limpa a tela

    for (int i = 0; i < menuSize; i++)
    {
        char line[20];
        if (i == menuIndex)
        {
            snprintf(line, sizeof(line), "> %s", menuEmojis[i]); // Indica opção selecionada
        }
        else
        {
            snprintf(line, sizeof(line), "%s", menuEmojis[i]);
        }
        int y = 10 + (i * LINE_SPACING); // Usa um espaçamento proporcional à fonte
        ssd1306_draw_string(ssd, 5, y, line);
    }

    char line[20];
    snprintf(line, sizeof(line), "Opcao = %d", menuIndex + 1);
    ssd1306_draw_string(ssd, 0, 56, line);

    render_on_display(ssd, frame_area);
}

void displayMenuTemperatura(uint8_t *ssd, struct render_area *frame_area, float temperatura)
{
    memset(ssd, 0, ssd1306_buffer_length); 

    char line[20];
    snprintf(line, sizeof(line), "Temperatura:%dC", 29);
    ssd1306_draw_string(ssd, 0, 10, line);
    // char umid[20];
    // snprintf(line, sizeof(line), "Umidade: %.2f", umidade);
    // ssd1306_draw_string(ssd, 0, 18, line);
    render_on_display(ssd, frame_area);
}

void displayMenuDistancia(uint8_t *ssd, struct render_area *frame_area, float distancia)
{
    memset(ssd, 0, ssd1306_buffer_length); 

    char line[20];
    snprintf(line, sizeof(line), "Distancia: %d cm", 40);
    ssd1306_draw_string(ssd, 0, 10, line);

    render_on_display(ssd, frame_area);
}

void displayMenuMusica(uint8_t *ssd, struct render_area *frame_area, int menuIndex)
{
    memset(ssd, 0, ssd1306_buffer_length);

    for (int i = 0; i < 2; i++)
    {
        char line[20];
        if (i == menuIndex)
        {
            snprintf(line, sizeof(line), "> %s", menuMusica[i]);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", menuMusica[i]);
        }
        int y = 10 + (i * LINE_SPACING);
        ssd1306_draw_string(ssd, 5, y, line);
    }

    char line[20];
    snprintf(line, sizeof(line), "Opcao = %d", menuIndex + 1);
    ssd1306_draw_string(ssd, 0, 56, line);

    render_on_display(ssd, frame_area);
}

void executarOpcao(int opcao)
{
    switch (opcao)
    {
    case 0:
        drawRostoFeliz();
        break;
    case 1:
        drawRostoTriste();
        break;
    case 2:
        drawRostoBravo();
        break;
    case 3:
        drawHeart();
        break;
    default:
        break;
    }
}

typedef struct
{
    float humidity;
    float temp_celsius;
} dht_reading;

void read_from_dht(dht_reading *result)
{
    int data[5] = {0, 0, 0, 0, 0};
    uint last = 1;
    uint j = 0;

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20);
    gpio_set_dir(DHT_PIN, GPIO_IN);

    for (uint i = 0; i < MAX_TIMINGS; i++) {
        uint count = 0;
        while (gpio_get(DHT_PIN) == last) {
            count++;
            sleep_us(1);
            if (count == 255) break;
        }
        last = gpio_get(DHT_PIN);
        if (count == 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (count > 16) data[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) {
            result->humidity = data[0];
        }
        result->temp_celsius = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (result->temp_celsius > 125) {
            result->temp_celsius = data[2];
        }
        if (data[2] & 0x80) {
            result->temp_celsius = -result->temp_celsius;
        }
    } else {
        printf("Bad data\n");
    }
}
//----------------------Menu de Opções-----------------------------------

int main()
{
    stdio_init_all();
    gpio_init(DHT_PIN);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);

    hc_sr04_init();
    buzzer_init();
    npInit(LED_PIN);
    npClear();
    npWrite();
    adc_init();

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    adc_gpio_init(JOY_Y);
    adc_gpio_init(JOY_X);
    ssd1306_init();

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    int opcao_atual = 0;
    int opcao_atual_emojis = 0;
    int opcao_atual_musica = 0;
    int currentMenu = -1;

    while (1)
    {
        adc_select_input(0);
        uint adc_y_raw = adc_read();
        adc_select_input(1);
        uint adc_x_raw = adc_read();

        switch (currentMenu)
        {
        case 0:

            displayMenuEmojis(ssd, &frame_area, opcao_atual_emojis);
            if (adc_y_raw < 1000)
            {
                if (opcao_atual_emojis == 3)
                    opcao_atual_emojis = 0;
                else
                    opcao_atual_emojis++;
                displayMenuEmojis(ssd, &frame_area, opcao_atual_emojis);
                sleep_ms(200);
            }
            else if (adc_y_raw > 2500)
            {
                if (opcao_atual_emojis == 0)
                    opcao_atual_emojis = 3;
                else
                    opcao_atual_emojis--;
                displayMenuEmojis(ssd, &frame_area, opcao_atual_emojis);
                sleep_ms(200);
            }

            if (!gpio_get(BUTTON_B))
            {
                exibir_matriz_led_Emojis(opcao_atual_emojis);
                sleep_ms(100);
            }
            if (!gpio_get(BUTTON_A))
            {
                opcao_atual_emojis = 0;
                currentMenu = -1;
                limparMatriz();
            }
            break;
        case 1:
            dht_reading reading;
            read_from_dht(&reading);
            displayMenuTemperatura(ssd, &frame_area, reading.temp_celsius);
            if (!gpio_get(BUTTON_A))
            {
                currentMenu = -1;
                limparMatriz();
            }
            break;
        case 2:
            //float distance = hc_sr04_read_distance();
            float distance = 30.0;
            displayMenuDistancia(ssd, &frame_area, distance);
            if (!gpio_get(BUTTON_A))
            {
                currentMenu = -1;
                limparMatriz();
            }

            break;
        case 3:
            displayMenuMusica(ssd, &frame_area, opcao_atual_musica);

            if (adc_y_raw < 1000)
            {
                if (opcao_atual_musica == 1)
                    opcao_atual_musica = 0;
                else
                {
                    opcao_atual_musica++;
                    displayMenuMusica(ssd, &frame_area, opcao_atual_musica);
                    sleep_ms(200);
                }
            }
            else if (adc_y_raw > 2500)
            {
                if (opcao_atual_musica == 0)
                    opcao_atual_musica = 1;
                else
                    opcao_atual_musica--;

                displayMenuMusica(ssd, &frame_area, opcao_atual_musica);
                sleep_ms(200);
            }

            if (!gpio_get(BUTTON_B))
            {
                exibir_matriz_led_Musica(opcao_atual_musica);
                sleep_ms(100);
            }
            if (!gpio_get(BUTTON_A))
            {
                opcao_atual_musica = 0;
                currentMenu = -1;
                limparMatriz();
            }
            break;
        default:
            displayMenu(ssd, &frame_area, opcao_atual);
            if (adc_y_raw < 1000)
            {
                if (opcao_atual == 3)
                    opcao_atual = 0;
                else
                    opcao_atual++;
                displayMenu(ssd, &frame_area, opcao_atual);
                sleep_ms(200);
            }
            else if (adc_y_raw > 2500)
            {
                if (opcao_atual == 0)
                    opcao_atual = 3;
                else
                    opcao_atual--;

                displayMenu(ssd, &frame_area, opcao_atual);
                sleep_ms(200);
            }

            if (!gpio_get(BUTTON_B))
            {
                exibir_matriz_led(opcao_atual);
                currentMenu = opcao_atual;
                sleep_ms(100);
            }
            if (!gpio_get(BUTTON_A))
            {

                currentMenu = -1;
                limparMatriz();
            }
            break;
        }

        sleep_ms(50);
    }
}
