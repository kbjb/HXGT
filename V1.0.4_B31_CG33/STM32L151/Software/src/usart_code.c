#include "usart_head.h"
#include <string.h>

/* [B1] UART3 remains disabled: only USART1/2 exist in the active board. */
/* [B2] ISR collects bytes into length-framed packets; application callbacks
 * run ONLY from UART_Process_Received() in the main loop. The live RX buffer
 * and the completed-frame mailbox are separate to avoid pointer reuse. */
static UART_Manager_t uart_managers[UART_MAX];
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

#define BT_BOOT_DELAY_MS 250U
#define BT_REPLY_TIMEOUT_MS 80U
#define BT_TX_TIMEOUT_MS 5U

static uint8_t bt_status = BT_UNKNOWN, bt_target, bt_step;
static uint8_t bt_waiting, bt_failed, bt_boot_delay, bt_exit_retry;
static uint32_t bt_tick;
static volatile uint8_t bt_listening, bt_reply, bt_reply_index;

uint8_t Bluetooth_Status(void) { return bt_status; }
uint8_t Bluetooth_Busy(void) { return bt_status == BT_BUSY && !bt_boot_delay; }

static void Bluetooth_Request(uint8_t enabled, uint8_t boot)
{
    bt_listening = 0U;
    bt_status = BT_BUSY;
    bt_target = enabled;
    bt_step = 0U;
    bt_waiting = 0U;
    bt_failed = 0U;
    bt_exit_retry = 0U;
    bt_boot_delay = boot;
    bt_tick = HAL_GetTick();
}

void Bluetooth_Startup(void)
{
    Bluetooth_Request(0U,1U);
}

void Bluetooth_Toggle(void)
{
    if (bt_status != BT_BUSY) Bluetooth_Request(bt_status != BT_ON,0U);
}

static uint8_t Bluetooth_Consume_Reply(uint8_t byte)
{
    static const char ok[] = "OK\r\n";
    if (!bt_listening || bt_reply) return 0U;
    if (byte == (uint8_t)ok[bt_reply_index])
    {
        if (++bt_reply_index == sizeof(ok)-1U) bt_reply = 1U;
        return 1U;
    }
    bt_reply_index = byte == 'O' ? 1U : 0U;
    return bt_reply_index != 0U;
}

void Bluetooth_Service(void)
{
    static const char * const on[]  = {"AT+ENAT\r\n","AT+LEON\r\n","AT+SPON\r\n","AT+EXAT\r\n"};
    static const char * const off[] = {"AT+ENAT\r\n","AT+LEOF\r\n","AT+SPOF\r\n","AT+EXAT\r\n"};
    uint8_t accepted;
    const char *command;
    if (bt_status != BT_BUSY) return;
    /* Leave USART2 available for PC traffic during module startup. */
    if (bt_boot_delay && (uint32_t)(HAL_GetTick()-bt_tick) < BT_BOOT_DELAY_MS) return;
    bt_boot_delay = 0U;

    if (bt_waiting)
    {
        accepted = bt_reply;
        if (!accepted && (uint32_t)(HAL_GetTick()-bt_tick) < BT_REPLY_TIMEOUT_MS) return;
        bt_listening = 0U;
        bt_waiting = 0U;
        if (!accepted) bt_failed = 1U;
        if (bt_step == 3U)
        {
            if (accepted || bt_exit_retry)
            {
                bt_status = bt_failed ? BT_ERROR : bt_target ? BT_ON : BT_OFF;
                return;
            }
            bt_exit_retry = 1U;
        }
        else
        {
            if (bt_step == 0U && !accepted) bt_step = 3U;
            else bt_step++;
        }
    }

    command = bt_target ? on[bt_step] : off[bt_step];
    bt_reply = 0U;
    bt_reply_index = 0U;
    bt_listening = 1U;
    bt_waiting = 1U;
    bt_tick = HAL_GetTick();
    if (HAL_UART_Transmit(&huart2,(uint8_t *)command,(uint16_t)strlen(command),BT_TX_TIMEOUT_MS) != HAL_OK)
        bt_tick -= BT_REPLY_TIMEOUT_MS;
}

static UART_ID_t UART_B2_Id(const UART_HandleTypeDef *huart)
{
    if (huart == NULL) return UART_MAX;
    if (huart->Instance == USART1) return UART1_ID;
    if (huart->Instance == USART2) return UART2_ID;
    return UART_MAX;
}

static void UART_B2_Reset_Parser(UART_Manager_t *manager)
{
    manager->rx_index = 0;
    manager->expected_length = 0;
}

static void UART_B2_Arm(UART_Manager_t *manager)
{
    if (manager->huart == NULL || manager->rx_armed) return;
    if (HAL_UART_Receive_IT(manager->huart, &manager->rx_byte, 1) == HAL_OK)
        manager->rx_armed = 1;
    else
        manager->error_count++;
}

void UART_Manager_Init(UART_ID_t uart_id)
{
    UART_Manager_t *manager;
    if (uart_id >= UART_MAX) return;
    manager = &uart_managers[uart_id];
    memset(manager, 0, sizeof(*manager));
    if (uart_id == UART1_ID) manager->huart = &huart1;
    else if (uart_id == UART2_ID) manager->huart = &huart2;
    /* [B1] No UART3 register access. */
    if (manager->huart != NULL)
        __HAL_UART_ENABLE_IT(manager->huart, UART_IT_IDLE);
}

void UART_Start_Receive(UART_ID_t uart_id)
{
    UART_Manager_t *manager;
    if (uart_id >= UART_MAX) return;
    manager = &uart_managers[uart_id];
    if (manager->huart == NULL) return;
    /* [B2] Start/recover reception only. Normal packet completion never
     * aborts HAL reception or clears a pending application frame. */
    HAL_UART_AbortReceive(manager->huart);
    manager->rx_armed = 0;
    UART_B2_Reset_Parser(manager);
    UART_B2_Arm(manager);
}

void UART_Send_Data(UART_ID_t uart_id, uint8_t *data, uint16_t length)
{
    UART_Manager_t *manager;
    if (uart_id >= UART_MAX || data == NULL || length == 0 ||
        length > UART_TX_BUFFER_SIZE) return;
    manager = &uart_managers[uart_id];
    if (manager->huart == NULL) return;
    /* [B2] Only called from foreground. Bound a failed TX instead of
     * indefinitely blocking the main loop with HAL_MAX_DELAY. */
    if (HAL_UART_Transmit(manager->huart, data, length, 100U) != HAL_OK)
        manager->error_count++;
}

uint8_t UART_Is_Frame_Ready(UART_ID_t uart_id)
{
    if (uart_id >= UART_MAX) return 0;
    return uart_managers[uart_id].frame_ready;
}

uint16_t UART_Get_Frame_Length(UART_ID_t uart_id)
{
    if (uart_id >= UART_MAX) return 0;
    return uart_managers[uart_id].frame_length;
}

uint8_t* UART_Get_Frame_Data(UART_ID_t uart_id)
{
    if (uart_id >= UART_MAX) return NULL;
    return uart_managers[uart_id].frame_buffer;
}

void UART_Clear_Frame_Flag(UART_ID_t uart_id)
{
    if (uart_id >= UART_MAX) return;
    uart_managers[uart_id].frame_ready = 0;
    uart_managers[uart_id].frame_length = 0;
}

void UART_Register_Frame_Callback(UART_ID_t uart_id,
        void (*callback)(uint8_t, uint8_t*, uint16_t))
{
    if (uart_id >= UART_MAX) return;
    uart_managers[uart_id].frame_received_callback = callback;
}

uint32_t UART_Get_Bad_Frame_Count(UART_ID_t uart_id)
{
    return uart_id < UART_MAX ? uart_managers[uart_id].bad_frame_count : 0;
}

uint32_t UART_Get_Dropped_Frame_Count(UART_ID_t uart_id)
{
    return uart_id < UART_MAX ? uart_managers[uart_id].dropped_frame_count : 0;
}

uint32_t UART_Get_Error_Count(UART_ID_t uart_id)
{
    return uart_id < UART_MAX ? uart_managers[uart_id].error_count : 0;
}

static void UART_B2_Consume_Byte(UART_ID_t uart_id, uint8_t c)
{
    UART_Manager_t *manager = &uart_managers[uart_id];
    uint16_t i, sum = 0;

    if (manager->rx_index == 0)
    {
        if (c != 0x5A) return;
        manager->rx_buffer[0] = c;
        manager->rx_index = 1;
        return;
    }
    if (manager->rx_index >= UART_RX_BUFFER_SIZE)
    {
        manager->bad_frame_count++;
        UART_B2_Reset_Parser(manager);
        return;
    }
    manager->rx_buffer[manager->rx_index++] = c;

    if (uart_id == UART1_ID && manager->rx_index == 2)
    {
        /* [B2] Sensor frame: 5A 0A + 8 payload + checksum + 0D 0A. */
        if (c != 0x0A)
        {
            manager->bad_frame_count++;
            UART_B2_Reset_Parser(manager);
            if (c == 0x5A) { manager->rx_buffer[0] = c; manager->rx_index = 1; }
            return;
        }
        manager->expected_length = 13;
    }
    else if (uart_id == UART2_ID && manager->rx_index == 4)
    {
        uint16_t data_len = (uint16_t)(((uint16_t)manager->rx_buffer[2] << 8) |
                                               manager->rx_buffer[3]);
        if (data_len > UART_RX_BUFFER_SIZE - 7U)
        {
            manager->bad_frame_count++;
            UART_B2_Reset_Parser(manager);
            return;
        }
        manager->expected_length = data_len + 7U;
    }

    if (manager->expected_length == 0 || manager->rx_index < manager->expected_length)
        return;

    /* [B2] Full-length frame, tail, and additive checksum are all mandatory
     * BEFORE either protocol callback can run. Payload CR/LF is not a tail. */
    if (manager->rx_buffer[manager->expected_length - 2U] != 0x0D ||
        manager->rx_buffer[manager->expected_length - 1U] != 0x0A)
    {
        manager->bad_frame_count++;
        UART_B2_Reset_Parser(manager);
        return;
    }
    for (i = 0; i < manager->expected_length - 3U; ++i)
        sum = (uint16_t)(sum + manager->rx_buffer[i]);
    if ((uint8_t)sum != manager->rx_buffer[manager->expected_length - 3U])
    {
        manager->bad_frame_count++;
        UART_B2_Reset_Parser(manager);
        return;
    }
    if (manager->frame_ready)
    {
        /* [B2] A one-frame mailbox is intentionally bounded. Report drop;
         * NEVER overwrite a frame which the foreground is still processing. */
        manager->dropped_frame_count++;
    }
    else
    {
        memcpy(manager->frame_buffer, manager->rx_buffer, manager->expected_length);
        manager->frame_length = manager->expected_length;
        manager->frame_ready = 1;
    }
    UART_B2_Reset_Parser(manager);
}

void UART_IDLE_IRQHandler(UART_HandleTypeDef *huart)
{
    UART_ID_t uart_id = UART_B2_Id(huart);
    if (uart_id == UART_MAX || uart_managers[uart_id].huart == NULL) return;
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        /* [B2] A gap interrupts a partial frame but does not discard any
         * completed packet. UART RX remains armed continuously. */
        if (uart_managers[uart_id].rx_index > 0)
        {
            uart_managers[uart_id].bad_frame_count++;
            UART_B2_Reset_Parser(&uart_managers[uart_id]);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    UART_ID_t uart_id = UART_B2_Id(huart);
    UART_Manager_t *manager;
    if (uart_id == UART_MAX) return;
    manager = &uart_managers[uart_id];
    if (manager->huart != huart) return;
    manager->rx_armed = 0;
    if (uart_id != UART2_ID || manager->rx_index != 0U ||
        !Bluetooth_Consume_Reply(manager->rx_byte))
        UART_B2_Consume_Byte(uart_id, manager->rx_byte);
    UART_B2_Arm(manager);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    UART_ID_t uart_id = UART_B2_Id(huart);
    if (uart_id < UART_MAX)
        uart_managers[uart_id].tx_busy = 0;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    UART_ID_t uart_id = UART_B2_Id(huart);
    UART_Manager_t *manager;
    if (uart_id == UART_MAX) return;
    manager = &uart_managers[uart_id];
    if (manager->huart != huart) return;
    manager->error_count++;
    /* [B2] Recover from overrun/noise/framing errors. Pending completed
     * frames remain available to the foreground. */
    UART_Start_Receive(uart_id);
}

void UART_Process_Received(UART_ID_t uart_id)
{
    UART_Manager_t *manager;
    if (uart_id >= UART_MAX) return;
    manager = &uart_managers[uart_id];
    if (manager->huart == NULL) return;
    if (!manager->rx_armed) UART_B2_Arm(manager);
    if (!manager->frame_ready) return;
    /* [B2] No protocol code and no blocking HAL_UART_Transmit in IRQ. */
    if (manager->frame_received_callback != NULL)
        manager->frame_received_callback(uart_id, manager->frame_buffer,
                                         manager->frame_length);
    UART_Clear_Frame_Flag(uart_id);
}

void UART_Init(void)
{
    UART_Manager_Init(UART1_ID);
    UART_Manager_Init(UART2_ID);
    /* [B1] UART3 remains reserved and disabled. */
    UART_Register_Frame_Callback(UART1_ID, Agreement_Ack_For_Uart1);
    UART_Register_Frame_Callback(UART2_ID, Agreement_Ack_For_Uart2);
    UART_Start_Receive(UART1_ID);
    UART_Start_Receive(UART2_ID);
}
