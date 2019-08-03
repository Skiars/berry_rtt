#include "be_readline.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <rtthread.h>

#define LINE_LENGTH     64
#define HISTORY_COUNT   8

#define QUEUE_SIZE      16
#define QUEUE_EMPTY     0x10000

struct shell {
    int linepos, linecount, newline;
    int hiscur, hiscnt;
    char line[LINE_LENGTH + 1];
    char history[HISTORY_COUNT][LINE_LENGTH + 1];
    const char *prompt;
#ifndef RT_USING_POSIX
    rt_device_t dev;
    rt_sem_t rx_sem;
    rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size);
#endif
};

static struct shell m_shell = { 0 };

#ifndef RT_USING_POSIX
static rt_err_t shell_rx_ind(rt_device_t dev, rt_size_t size)
{
    /* release semaphore to let finsh thread rx data */
    rt_sem_release(m_shell.rx_sem);
    return RT_EOK;
}
static void wait_char_init(struct shell *shell)
{
    if (shell->dev == RT_NULL) {
        shell->dev = rt_console_get_device();
        shell->rx_sem = rt_sem_create("be_sem", 0, 0);
        shell->rx_ind = shell->dev->rx_indicate;
        rt_device_set_rx_indicate(shell->dev, shell_rx_ind);
    }
}
#endif

static int wait_char(struct shell *shell)
{
#ifdef RT_USING_POSIX
    return getchar();
#else
    char ch = 0;
    wait_char_init(shell);
    while (rt_device_read(shell->dev, -1, &ch, 1) != 1) {
        rt_sem_take(shell->rx_sem, RT_WAITING_FOREVER);
    }
    return (int)ch;
#endif
}

void pack_line(struct shell *shell)
{
    shell->line[shell->linecount] = '\0';
    if (shell->linecount) {
        int hiscur = shell->hiscnt % HISTORY_COUNT;
        strcpy(shell->history[hiscur], shell->line);
        shell->hiscur = ++shell->hiscnt;
    }
    shell->linecount = shell->linepos = 0;
}

static void remove_char(struct shell *shell)
{
    int pos = --shell->linepos, count = shell->linecount;
    while (pos < count) {
        shell->line[pos] = shell->line[pos + 1];
        ++pos;
    }
    shell->line[--shell->linecount] = '\0';
}

static void insert_char(struct shell *shell, int ch)
{
    int pos = shell->linepos, count = shell->linecount;
    if (count < LINE_LENGTH) {
        while (count >= pos) {
            shell->line[count + 1] = shell->line[count];
            --count;
        }
        shell->line[pos] = ch;
        ++shell->linecount;
        ++shell->linepos;
        for (count = shell->linecount; pos < count; ++pos) {
            rt_kprintf("%c", shell->line[pos]);
        }
        for (count -= shell->linepos; count > 0; --count) {
            rt_kprintf("\b");
        }
    }
}

static void backspace(struct shell *shell)
{
    if (shell->linepos > 0) {
        remove_char(shell);
        if (shell->linepos < shell->linecount) {
            int i;
            rt_kprintf("\b%s \b", shell->line + shell->linepos);
            for (i = shell->linepos; i < shell->linecount; ++i) {
                rt_kprintf("\b");
            }
        } else {
            rt_kprintf("\b \b");
        }
    }
}

static int addchar(struct shell *shell, int ch)
{
    switch (ch) {
    case '\x04': /* Ctrl+D */
        if (!shell->linecount) {
#ifndef RT_USING_POSIX
            rt_device_set_rx_indicate(shell->dev, shell->rx_ind);
            shell->dev = NULL;
            rt_sem_delete(shell->rx_sem);
#endif
            return 2;
        }
        break;
    case '\r':
        pack_line(shell);
        rt_kprintf("\n");
        shell->newline = 1;
        return 1;
    case '\n':
        if (shell->newline) {
            shell->newline = 0;
            break;
        }
        pack_line(shell);
        return 1;
    case '\b': case '\177':
        backspace(shell);
        break;
    default:
        if (isprint(ch)) {
            insert_char(shell, ch);
        }
    }
    return 0;
}

static void change_history(struct shell *shell)
{
    int pos = shell->hiscur % HISTORY_COUNT;
    char *line = shell->history[pos];
    strcpy(shell->line, line);
    rt_kprintf("\r%s%s\033[K", shell->prompt, line);
    shell->linecount = shell->linepos = strlen(line);
}

static void up_key(struct shell *shell)
{
    if (shell->hiscur > 0 && shell->hiscur
            + HISTORY_COUNT > shell->hiscnt) {
        --shell->hiscur;
        change_history(shell);
    }
}

static void down_key(struct shell *shell)
{
    if (shell->hiscur + 1 < shell->hiscnt) {
        ++shell->hiscur;
        change_history(shell);
    }
}

static void left_key(struct shell *shell)
{
    if (shell->linepos > 0) {
        --shell->linepos;
        rt_kprintf("\b");
    }
}

static void right_key(struct shell *shell)
{
    if (shell->linepos < shell->linecount) {
        rt_kprintf("%c", shell->line[shell->linepos++]);
    }
}

const char* p_readline(struct shell *shell)
{
    int state = 0;
    for (;;) {
        int ch = wait_char(shell);
        if (ch == 0x1B) {
            state = 1;
        } else if (state == 1) {
            state = ch == 0x5B ? 2 : 0;
        } else if (state == 2) {
            state = 0;
            switch (ch) {
                case 0x41: up_key(shell); break;
                case 0x42: down_key(shell); break;
                case 0x43: right_key(shell); break;
                case 0x44: left_key(shell); break;
                default: break;
            }
        } else {
            switch (addchar(shell, ch)) {
                case 1: return shell->line;
                case 2: return NULL;
                default: break;
            }
        }
        shell->newline = 0;
    }
}

const char* berry_readline(const char *prompt)
{
    m_shell.prompt = prompt;
    rt_kprintf("%s", prompt);
    return p_readline(&m_shell);
}
