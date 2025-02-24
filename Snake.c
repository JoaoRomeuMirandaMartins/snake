#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define GPIO_VRy 26
#define GPIO_VRx 27
#define ADC_VRy 0
#define ADC_VRx 1
#define FOOD_CODE 2
#define SNAKE_CODE 1
#define VOID_CODE 0

#define FOOD_SPRITE {0b0000, 0b0110, 0b0110, 0b0000}
#define SNAKE_SPRITE {0b1111, 0b1111, 0b1111, 0b1111}
#define VOID_SPRITE {0b0000, 0b0000, 0b0000, 0b0000}

#define OLED_ADD 0x3C
#define COMMAND_REG 0x80
#define DATA_REG 0x40

#define X 0
#define Y 1

#define RIGHT 0
#define DOWN 1
#define LEFT 2
#define UP 3

uint8_t snakeMAP[32][16];
typedef struct snakeType {
    uint8_t headX;
    uint8_t headY;
    uint8_t direction;
    uint8_t body[512][2];
    uint16_t size;
    uint16_t growth;

}snakeType;

snakeType snake;

uint8_t foodX;
uint8_t foodY;

uint8_t sprites[3][4] = {VOID_SPRITE, SNAKE_SPRITE, FOOD_SPRITE};

void drawFood() {
    while (snakeMAP[foodX][foodY] != 0x00) {
        foodX = get_rand_32() >> 27;
        foodY = get_rand_32() >> 28;
    }
    snakeMAP[foodX][foodY] = FOOD_CODE;
    setCursor(foodX << 2, foodY >> 1);
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (sprites[FOOD_CODE][0] << ((foodY & 0x01) << 2)) | (sprites[snakeMAP[foodX][((foodY & 0x01) ^ 0x01)]][0] << (((foodY & 0x01) ^ 0x01) << 2)));
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (sprites[FOOD_CODE][1] << ((foodY & 0x01) << 2)) | (sprites[snakeMAP[foodX][((foodY & 0x01) ^ 0x01)]][1] << (((foodY & 0x01) ^ 0x01) << 2)));
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (sprites[FOOD_CODE][2] << ((foodY & 0x01) << 2)) | (sprites[snakeMAP[foodX][((foodY & 0x01) ^ 0x01)]][2] << (((foodY & 0x01) ^ 0x01) << 2)));
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (sprites[FOOD_CODE][3] << ((foodY & 0x01) << 2)) | (sprites[snakeMAP[foodX][((foodY & 0x01) ^ 0x01)]][3] << (((foodY & 0x01) ^ 0x01) << 2)));
};

void i2cWrite(port, devadd, regadd, value){
    uint8_t data[2];
    data[0] = regadd;
    data[1] = value;
    i2c_write_blocking(port, devadd, &data, 2, true);
}
/*
uint8_t i2cread(uint8_t port, uint8_t devadd, uint8_t regadd){
    uint8_t regval;
    i2c_write_blocking(I2C_PORT, devadd, &regadd, 1, true);
    i2c_read_blocking(I2C_PORT, devadd, &regval, 1, false);
    return regval;
}
*/

void setCursor(uint8_t cursorX, uint8_t cursorY){
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, (cursorX & 0x0F));
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x10 | ((cursorX >> 4) & 0x0F));
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0xB0 | (cursorY & 0x0F));
}

void clearDisplay(){
    uint8_t cont;
    uint8_t contII;
    for (cont = 0; cont < 8; cont++) {
        i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x00);
        i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x10);
        i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0xB0 | cont);

        for (contII = 0; contII < 128; contII++) {
            i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, 0x00);
        }
    }
}

/*void oledPrint(){
    uint8_t cont;
    uint8_t contII;
    for (cont = 0; cont < 16; cont = cont + 2) {
        i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x00);
        i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x10);
        i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0xB0 | (cont >> 1));// (cont >> 1) means (cont / 2)
        
        for (contII = 0; contII < 32; contII++) {
            i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (snakeMAP[contII][cont + 1] << 4) | snakeMAP[contII][cont]);
            i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (snakeMAP[contII][cont + 1] << 4) | snakeMAP[contII][cont]);
            i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (snakeMAP[contII][cont + 1] << 4) | snakeMAP[contII][cont]);
            i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, (snakeMAP[contII][cont + 1] << 4) | snakeMAP[contII][cont]);
        }
    }
}*/

void mapUpdate(posX, posY, active){
    snakeMAP[posX][posY] = active;
    setCursor((posX << 2), (posY >> 1));
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, ((sprites[snakeMAP[posX][posY | 0x01]][0]) << 4) | sprites[snakeMAP[posX][posY & 0xFE]][0]);
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, ((sprites[snakeMAP[posX][posY | 0x01]][1]) << 4) | sprites[snakeMAP[posX][posY & 0xFE]][1]);
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, ((sprites[snakeMAP[posX][posY | 0x01]][2]) << 4) | sprites[snakeMAP[posX][posY & 0xFE]][2]);
    i2cWrite(I2C_PORT, OLED_ADD, DATA_REG, ((sprites[snakeMAP[posX][posY | 0x01]][3]) << 4) | sprites[snakeMAP[posX][posY & 0xFE]][3]);
}

uint8_t move(direction) {
    uint16_t cont;
    uint8_t gameOver = false;
    printf("snake.headX:%i snake.headY:%i direction:%i\n", snake.headX, snake.headY, snake.direction);
    if (!((snake.headY == 0 && direction == DOWN) || (snake.headX == 0 && direction == RIGHT))) {
        switch (direction) {
        case RIGHT:
            snake.headX--;
            break;
        case DOWN:
            snake.headY--;
            break;
        case LEFT:
            snake.headX++;
            break;
        case UP:
            snake.headY++;
        }
        if (snake.headX == 32) {
            gameOver = true;
        }
        if (snake.headY == 16) {
            gameOver = true;
        }
        if (!gameOver){
            if (snakeMAP[snake.headX][snake.headY] == FOOD_CODE){
                snake.growth++;
                drawFood();
            };
            for (cont = snake.size + 1; cont > 0; cont--) {
                snake.body[cont][X] = snake.body[cont - 1][X];
                snake.body[cont][Y] = snake.body[cont - 1][Y];
            }
            snake.body[0][X] = snake.headX;
            snake.body[0][Y] = snake.headY;
            if (snake.growth == 0) {
                //printf("Deleting (%i, %i)", snake.body[snake.size][X], snake.body[snake.size][Y]);
                mapUpdate(snake.body[snake.size][X], snake.body[snake.size][Y], VOID_CODE);
            }else{
                snake.size++;
                snake.growth--;
            }
            gameOver = snakeMAP[snake.headX][snake.headY] == SNAKE_CODE;
            mapUpdate(snake.headX, snake.headY, SNAKE_CODE);
        }
    }else{
        gameOver = true;
    }
    return gameOver;
}

int main() {
    uint8_t gameOver;
    uint8_t oldDirection;
    uint16_t VRy;
    uint16_t VRx;
    snake.headX = 5;
    snake.headY = 5;
    snake.direction = LEFT;
    snake.size = 3;
    snake.growth = 0;
    snake.body[0][X] = 5;
    snake.body[0][Y] = 5;
    snake.body[1][X] = 4;
    snake.body[1][Y] = 5;
    snake.body[2][X] = 3;
    snake.body[2][Y] = 5;
    snake.body[3][X] = 2;
    snake.body[3][Y] = 5;

    stdio_init_all();

    adc_init();
    adc_gpio_init(GPIO_VRx);
    adc_gpio_init(GPIO_VRy);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    //Starting Display
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0xAF);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0xA6);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x20);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x02);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x8D);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x14);
    
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x00);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0x10);
    i2cWrite(I2C_PORT, OLED_ADD, COMMAND_REG, 0xB0);
    
    //Clearing Display
    clearDisplay();
    sleep_ms(5000);
    
    //ClearMAP
    uint16_t cont;
    uint8_t contII;
    for (cont = 0; cont < 16; cont++) {
        for (contII = 0; contII < 32; contII++) {
            snakeMAP[contII][cont] = 0;
        }
    }

    for (cont = 0; cont < snake.size; cont++){
        snakeMAP[snake.body[cont][X]][snake.body[cont][Y]] = SNAKE_CODE;
    }

    for(cont = 0; cont < snake.size; cont++){
        mapUpdate(snake.body[cont][X], snake.body[cont][Y], SNAKE_CODE);
    }

    drawFood();

    while (true) {
        adc_select_input(ADC_VRy);
        VRy = adc_read();
        adc_select_input(ADC_VRx);
        VRx = adc_read();
        oldDirection = snake.direction;
        if (VRx > 3500 & oldDirection != LEFT) {
            snake.direction = RIGHT;
        }else if (VRx < 500 & oldDirection != RIGHT) {
            snake.direction = LEFT;
        }
        if (VRy > 3500 & oldDirection != DOWN) {
            snake.direction = UP;
        }else if (VRy < 500 & oldDirection != UP) {
            snake.direction = DOWN;
        }
        gameOver = (!gameOver)? move(snake.direction):true;
        sleep_ms(750);
    }
}
