#include "shell.h"

#include <string.h>

#include "adc.h"
#include "app.h"
#include "config.h"
#include "timer.h"
#include "uart.h"

#if SHELL_LINE_SIZE > 32
#error "SHELL_LINE_SIZE must be <= 32"
#endif

#ifndef SHELL_HELP_ENABLE
#define SHELL_HELP_ENABLE 1  // SHELL 帮助信息是否打印，需要 delay_ms 支持
#endif

/* ============================================================
 * 命令表
 * ============================================================ */

typedef void (*shell_cmd_fn)(int argc, char** argv);

typedef struct {
  const char* cmd;
  shell_cmd_fn handler;
} shell_cmd_t;

static void cmd_rst(int argc, char** argv);
static void cmd_debug(int argc, char** argv);
static void cmd_time(int argc, char** argv);
static void cmd_tx(int argc, char** argv);
static void cmd_rx(int argc, char** argv);
static void cmd_adc(int argc, char** argv);
static void cmd_app(int argc, char** argv);

static xdata const shell_cmd_t cmd_table[] = {
    {"rst", cmd_rst}, {"debug", cmd_debug}, {"time", cmd_time}, {"tx", cmd_tx}, {"rx", cmd_rx}, {"adc", cmd_adc}, {"app", cmd_app}, {NULL, NULL},
};

/* ============================================================
 * Shell 状态机
 * ============================================================ */

typedef enum {
  SHELL_ST_INPUT = 0,
  SHELL_ST_EXEC,
  SHELL_ST_ESC_IGNORE,
} shell_state_t;

typedef enum {
  SHELL_EVT_NONE = 0,
  SHELL_EVT_CHAR,
  SHELL_EVT_BACKSPACE,
  SHELL_EVT_ENTER,
  SHELL_EVT_ESC,
} shell_event_t;

static xdata shell_state_t s_state = 0;
static xdata uint8_t s_line[SHELL_LINE_SIZE] = {0};
static xdata uint8_t s_line_len = 0;
static xdata uint8_t s_esc_cnt = 0;
static xdata uint8_t s_ignore_lf = 0;

/* ============================================================
 * 命令实现
 * ============================================================ */

static void cmd_rst(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);
  if (app_post_event(APP_EVT_RESTART)) {
    printf("restarting after 2s...\r\n");
  } else {
    printf("failed to queue restart event\r\n");
  }
}

static void cmd_time(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);
  printf("time: %lu ms\r\n", (unsigned long)timer_get_ms());
}

static void cmd_tx(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);
  printf("tx: %u/%u, drop=%u\r\n", (unsigned int)uart_tx_used(), (unsigned int)UART_TX_BUFF_SIZE, (unsigned int)uart_tx_drop());
}

static void cmd_rx(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);
  printf("rx: %u/%u, drop=%u\r\n", (unsigned int)uart_rx_used(), (unsigned int)UART_RX_BUFF_SIZE, (unsigned int)uart_rx_drop());
}

static void cmd_adc(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);
  printf("adc: P5.4=%u P5.5=%u\r\n", (unsigned int)adc_last_value(ADC_CHANNEL_P54), (unsigned int)adc_last_value(ADC_CHANNEL_P55));
}

static void cmd_debug(int argc, char** argv) {
  if (argc == 1) {
    printf("debug: %s\r\n", (app_get_state() == APP_ST_DEBUG) ? "ON" : "OFF");
    return;
  }
  if (argc == 2 && strcmp(argv[1], "enter") == 0) {
    if (app_post_event(APP_EVT_DEBUG_ENTER)) {
      printf("ENTER debug mode\r\n");
    } else {
      printf("event queue full\r\n");
    }
    return;
  }
  if (argc == 2 && strcmp(argv[1], "exit") == 0) {
    if (app_post_event(APP_EVT_DEBUG_EXIT)) {
      printf("EXIT debug mode\r\n");
    } else {
      printf("event queue full\r\n");
    }
    return;
  }
  printf("usage: debug [enter|exit]\r\n");
}

static void cmd_app(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);
  printf("APP state = %s\r\n", app_state_name(app_get_state()));
}

/* ============================================================
 * 命令行解析与执行
 * ============================================================ */

static void shell_exec_line(void) {
  char* argv[4];
  int argc = 0;
  uint8_t i = 0;
  uint8_t n = s_line_len;

  /* ---- 分词 ---- */
  while (i < n && argc < 4) {
    while (i < n && s_line[i] == ' ') i++;
    if (i >= n) break;

    argv[argc++] = (char*)&s_line[i];

    while (i < n && s_line[i] != ' ') i++;
    if (i < n) s_line[i++] = '\0';
  }

  /* ---- 查找并执行 ---- */
  if (argc > 0) {
    uint8_t k;
    for (k = 0; cmd_table[k].cmd != NULL; k++) {
      if (strcmp(argv[0], cmd_table[k].cmd) == 0) {
        cmd_table[k].handler(argc, argv);
        break;
      }
    }
    if (cmd_table[k].cmd == NULL) {
      printf("unknown command: %s\r\n", argv[0]);
    }
  }

  /* ---- 复位状态，准备下一行 ---- */
  s_line_len = 0;
  printf("> ");
  s_state = SHELL_ST_INPUT;
}

/* ============================================================
 * 初始化
 * ============================================================ */

void shell_init(void) {
  s_line_len = 0;
  s_esc_cnt = 0;
  s_ignore_lf = 0;
  s_state = SHELL_ST_INPUT;

#if SHELL_HELP_ENABLE == 1
  delay_ms(10);
  printf("commands help: \r\n");
  printf("  rst   : software reset\r\n");
  printf("  time  : show uptime\r\n");
  delay_ms(10);
  printf("  tx    : show uart tx stats\r\n");
  printf("  rx    : show uart rx stats\r\n");
  delay_ms(10);
  printf("  adc   : read current adc value\r\n");
  printf("  app   : show app state\r\n");
  delay_ms(10);
#endif

  printf("> ");
}

/* ============================================================
 * 主循环轮询
 * ============================================================ */

void shell_poll(uint32_t now) {
  uint8_t b;
  UNUSED(now);

  while (uart_read_byte(&b)) {
    shell_event_t evt = SHELL_EVT_NONE;

    /* ---- ESC 忽略状态 ---- */
    if (s_state == SHELL_ST_ESC_IGNORE) {
      if (s_esc_cnt > 0) {
        s_esc_cnt--;
        if (s_esc_cnt == 0) {
          s_state = SHELL_ST_INPUT;
        }
      }
      continue;
    }

    /* ---- 吞掉 \r 后紧跟的 \n ---- */
    if (s_ignore_lf) {
      s_ignore_lf = 0;
      if (b == 0x0A) continue;
    }

    /* ---- 字节 -> 事件 ---- */
    if (b == 0x08 || b == 0x7F) {
      evt = SHELL_EVT_BACKSPACE;
    } else if (b == 0x0D) {
      evt = SHELL_EVT_ENTER;
      s_ignore_lf = 1;
    } else if (b == 0x0A) {
      evt = SHELL_EVT_ENTER;
    } else if (b == 0x1B) {
      evt = SHELL_EVT_ESC;
    } else if (b >= 0x20 && b <= 0x7E) {
      evt = SHELL_EVT_CHAR;
    }

    /* ---- 状态迁移 ---- */
    switch (evt) {
      case SHELL_EVT_CHAR:
        if (s_line_len < SHELL_LINE_SIZE - 1) {
          s_line[s_line_len++] = b;
          putchar((char)b);
        }
        break;

      case SHELL_EVT_BACKSPACE:
        if (s_line_len > 0) {
          s_line_len--;
          printf("\b \b");
        }
        break;

      case SHELL_EVT_ENTER:
        printf("\r\n");
        s_line[s_line_len] = '\0';
        s_state = SHELL_ST_EXEC;
        shell_exec_line();
        break;

      case SHELL_EVT_ESC:
        s_state = SHELL_ST_ESC_IGNORE;
        s_esc_cnt = 2;
        break;

      default:
        break;
    }
  }
}
