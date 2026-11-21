#include "ecSTM32F4v2.h"
#include "ecUART2.h"
#include <stdint.h>
#include <stdio.h>

#define LED_BIT       (1UL << 5U)

#define RX_SIZE       512U
#define RX_MASK       (RX_SIZE - 1U)

#define LINE_SIZE     128U
#define RX_DAMAGED    0x100U

#define UART_ERRORS   (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)
#define BAD_BYTE      (USART_SR_FE | USART_SR_NE | USART_SR_PE)

/*
 * 각 USART의 수신 인터럽트가 데이터를 넣고,
 * main()에서 데이터를 꺼내는 수신 큐.
 *
 * RX_DAMAGED 비트는 수신 오류나 데이터 유실을 표시한다.
 */
typedef struct {
    volatile uint16_t data[RX_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t damaged;
} RxQueue;

/*
 * 문장 조립용 버퍼.
 * 최대 127바이트 + 문자열 끝을 나타내는 '\0'.
 */
typedef struct {
    uint8_t text[LINE_SIZE];
    uint16_t len;
    uint8_t was_cr;
    uint8_t discard;
} LineBuffer;

/* 내 PC 입력과 상대 MCU 수신을 각각 따로 저장 */
static RxQueue pc_rx;
static RxQueue peer_rx;

static LineBuffer pc_line;
static LineBuffer peer_line;


/* ==================== 수신 큐 ==================== */

static void rx_push(RxQueue *queue, uint8_t byte)
{
    uint16_t head = queue->head;
    uint16_t next = (uint16_t)((head + 1U) & RX_MASK);

    if (next == queue->tail) {
        queue->damaged = 1U;
        return;
    }

    queue->data[head] = (uint16_t)(byte |
        (queue->damaged != 0U ? RX_DAMAGED : 0U));

    queue->damaged = 0U;
    queue->head = next;
}

static uint8_t rx_pop(RxQueue *queue, uint16_t *item)
{
    uint16_t tail = queue->tail;

    if (tail == queue->head) {
        return 0U;
    }

    *item = queue->data[tail];
    queue->tail = (uint16_t)((tail + 1U) & RX_MASK);

    return 1U;
}


/* ==================== USART 수신 인터럽트 ==================== */

/*
 * 인터럽트에서는 수신 데이터만 읽어서 큐에 저장한다.
 * 전송과 printf()는 main()에서 실행한다.
 */
static void capture_rx(USART_TypeDef *uart, RxQueue *queue)
{
    uint32_t status = uart->SR;

    if ((status & (USART_SR_RXNE | UART_ERRORS)) != 0U) {
        uint8_t byte = (uint8_t)(uart->DR & 0xFFU);

        if ((status & UART_ERRORS) != 0U) {
            queue->damaged = 1U;
        }

        if ((status & USART_SR_RXNE) != 0U &&
            (status & BAD_BYTE) == 0U) {
            rx_push(queue, byte);
        }

        /*
         * ORE는 DR에 남아 있는 바이트 이후에 데이터가
         * 유실됐다는 의미이므로 다음 바이트에도 표시한다.
         */
        if ((status & USART_SR_ORE) != 0U) {
            queue->damaged = 1U;
        }
    }
}

void USART2_IRQHandler(void)
{
    /* 내 PC -> 내 MCU */
    capture_rx(USART2, &pc_rx);
}

void USART1_IRQHandler(void)
{
    /* 상대 MCU -> 내 MCU */
    capture_rx(USART1, &peer_rx);
}


/* ==================== 완성된 문장 처리 ==================== */

/*
 * 정상적인 문장이 완성됐을 때만 호출한다.
 *
 * from_pc == 1 : 내 PC에서 작성한 문장
 * from_pc == 0 : 상대 MCU에서 받은 문장
 */
static void finish_line(LineBuffer *line, uint8_t from_pc)
{
    if (from_pc != 0U) {
        /*
         * Enter를 눌렀을 때만 상대 MCU로 전송한다.
         * 문장 뒤에 '\n'을 보내 문장의 끝을 알린다.
         */
        uint8_t terminator = '\r';

        USART1_write(line->text, line->len);
        USART1_write(&terminator, 1U);
    }
    else {
        /* 상대가 보낸 문장을 한 번만 출력 */
        printf("RX: %s\r\n", (char *)line->text);

        /*
         * 완성된 메시지가 정확히 H 또는 L일 때만
         * 이 보드의 LED를 제어한다.
         */
        if (line->len == 1U && line->text[0] == 'H') {
            GPIOA->BSRR = LED_BIT;          /* LD2 ON */
        }
        else if (line->len == 1U && line->text[0] == 'L') {
            GPIOA->BSRR = LED_BIT << 16U;   /* LD2 OFF */
        }
    }
}


/* ==================== 바이트를 모아 문장 만들기 ==================== */

static void process_byte(LineBuffer *line,
                         uint16_t item,
                         uint8_t from_pc)
{
    uint8_t byte = (uint8_t)item;

    /* 수신 오류가 있었던 문장은 끝까지 버린다. */
    if ((item & RX_DAMAGED) != 0U) {
        line->discard = 1U;
        line->was_cr = 0U;
    }

    /*
     * Enter가 CR+LF로 들어와도 한 번만 처리한다.
     * CR만 또는 LF만 들어오는 경우도 처리한다.
     */
    if (byte == '\n' && line->was_cr != 0U) {
        line->was_cr = 0U;
        return;
    }

    line->was_cr = (byte == '\r') ? 1U : 0U;

    /* Enter 또는 상대 메시지의 끝을 받았을 때 */
    if (byte == '\r' || byte == '\n') {
        if (from_pc != 0U) {
            printf("\r\n");
        }

        /* %s로 출력할 수 있도록 문자열 종료 */
        line->text[line->len] = '\0';

        if (line->discard != 0U) {
            printf(
                "[%s line discarded: too long or RX error. Resend.]\r\n",
                from_pc != 0U ? "PC" : "Peer"
            );
        }
        else if (line->len != 0U) {
            finish_line(line, from_pc);
        }

        /* 다음 문장을 받을 준비 */
        line->len = 0U;
        line->discard = 0U;
        return;
    }

    /* 길이 초과 또는 수신 오류가 발생한 문장의 나머지 폐기 */
    if (line->discard != 0U) {
        return;
    }

    /*
     * 내 PC 입력의 Backspace 처리.
     * BS와 DEL 모두 지원하며, 바이트 단위로 삭제한다.
     */
    if (from_pc != 0U && (byte == '\b' || byte == 127U)) {
        if (line->len != 0U) {
            line->len--;
            printf("\b \b");
        }
        return;
    }

    /* 탭을 제외한 다른 ASCII 제어 문자는 저장하지 않는다. */
    if (byte < 32U && byte != '\t') {
        return;
    }

    if (byte == 127U) {
        return;
    }

    /* 마지막 한 칸은 '\0'을 위해 남긴다. */
    if (line->len >= LINE_SIZE - 1U) {
        line->discard = 1U;
        return;
    }

    /* 문장 버퍼에 저장 */
    line->text[line->len++] = byte;

    /*
     * 입력 중에는 내 PC에만 에코한다.
     * 여기서는 USART1으로 보내지 않는다.
     */
    if (from_pc != 0U) {
        USART2_write(&byte, 1U);
    }
}


/* ==================== 초기 설정 ==================== */

static void setup(void)
{
    RCC_PLL_init();
    SysTick_init();

    /* LD2: PA5 출력 설정 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->BSRR = LED_BIT << 16U;       /* 처음에는 LED OFF */

    GPIOA->MODER =
        (GPIOA->MODER & ~(3UL << 10U)) | (1UL << 10U);

    GPIOA->OTYPER &= ~LED_BIT;          /* Push-pull */
    GPIOA->PUPDR &= ~(3UL << 10U);      /* No pull-up/down */
    GPIOA->OSPEEDR &= ~(3UL << 10U);    /* Low speed */

    /* 기존에 통신에 성공한 초기화 함수 유지 */
    USART2_init();                     /* 내 PC */
    USART1_init();                     /* 상대 MCU */
}


/* ==================== main ==================== */

int main(void)
{
    uint16_t item;

    setup();

    printf("\r\nMCU Initialized\r\n");
    printf("Type a message, then Enter (max 127 bytes).\r\n");
    printf("H + Enter: peer LED ON; L + Enter: peer LED OFF.\r\n");

    while (1) {
        /* 상대 MCU에서 들어온 데이터 처리 */
        if (rx_pop(&peer_rx, &item) != 0U) {
            process_byte(&peer_line, item, 0U);
        }

        /* 내 PC에서 들어온 데이터 처리 */
        if (rx_pop(&pc_rx, &item) != 0U) {
            process_byte(&pc_line, item, 1U);
        }
    }
}