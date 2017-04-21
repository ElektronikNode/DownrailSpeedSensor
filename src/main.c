/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "string.h"

#include "ch.h"
#include "hal.h"
#include "ch_test.h"

#include "chprintf.h"
#include "shell.h"

#include "ff.h"

#include "ex/display/SSD1306.h"

/*===========================================================================*/
/* OLED Display                                                              */
/*===========================================================================*/

static FramebufferSW fb;

static const I2CConfig i2ccfg = {
		OPMODE_I2C,
		400000,
		FAST_DUTY_CYCLE_2,
};

static SSD1306Config ssd1306cfg = {
		&I2CD1,
		&i2ccfg,
		&fb,
		SSD1306_SA0_GND
};

static SSD1306Driver SSD1306D1;

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static virtual_timer_t tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static event_source_t inserted_event, removed_event;

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = p;

  chSysLockFromISR();
  if (cnt > 0) {
    if (blkIsInserted(bbdp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = POLLING_INTERVAL;
  }
  else {
    if (!blkIsInserted(bbdp)) {
      cnt = POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, bbdp);
  chSysUnlockFromISR();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init(void *p) {

  chEvtObjectInit(&inserted_event);
  chEvtObjectInit(&removed_event);
  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, p);
  chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fs_ready = FALSE;

/* Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig hs_spicfg = {NULL, IOPORT2, GPIOB_SPI2NSS, 0, 0};

/* Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig ls_spicfg = {NULL, IOPORT2, GPIOB_SPI2NSS,
                              SPI_CR1_BR_2 | SPI_CR1_BR_1,
                              0};

/* MMC/SD over SPI driver configuration.*/
static MMCConfig mmccfg = {&SPID2, &ls_spicfg, &hs_spicfg};

/* Generic large buffer.*/
static uint8_t fbuff[1024];

static FRESULT scan_files(BaseSequentialStream *chp, char *path) {
  static FILINFO fno;
  FRESULT res;
  DIR dir;
  size_t i;
  char *fn;

  res = f_opendir(&dir, path);
  chprintf(chp, "%s\r\n", path);
  if (res == FR_OK) {
    i = strlen(path);
    while (((res = f_readdir(&dir, &fno)) == FR_OK) && fno.fname[0]) {
      if (_FS_RPATH && fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        *(path + i) = '/';
        strcpy(path + i + 1, fn);
        res = scan_files(chp, path);
        *(path + i) = '\0';
        if (res != FR_OK)
          break;
      }
      else {
        chprintf(chp, "%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}

static FRESULT stat_file(BaseSequentialStream *chp, char *path) {
	FRESULT res;
	FILINFO fno;

	res = f_stat(path, &fno);
	switch (res) {

	case FR_OK:
		chprintf(chp, "Size: %lu\n", fno.fsize);
		chprintf(chp, "Timestamp: %u/%02u/%02u, %02u:%02u\n",
			   (fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
			   fno.ftime >> 11, fno.ftime >> 5 & 63);
		chprintf(chp, "Attributes: %c%c%c%c%c\n",
			   (fno.fattrib & AM_DIR) ? 'D' : '-',
			   (fno.fattrib & AM_RDO) ? 'R' : '-',
			   (fno.fattrib & AM_HID) ? 'H' : '-',
			   (fno.fattrib & AM_SYS) ? 'S' : '-',
			   (fno.fattrib & AM_ARC) ? 'A' : '-');
		break;

	case FR_NO_FILE:
		chprintf(chp, "File does not exist.\n");
		break;

	default:
		chprintf(chp, "An error occured. (%d)\n", res);
	}

	return res;
}

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static void cmd_tree(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: tree\r\n");
    return;
  }
  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }
  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getfree() failed\r\n");
    return;
  }
  chprintf(chp,
           "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
           clusters, (uint32_t)SDC_FS.csize,
           clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
  fbuff[0] = 0;
  scan_files(chp, (char *)fbuff);
}

static void cmd_stat(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  if (argc != 1) {
    chprintf(chp, "Usage: stat <path>\r\n");
    return;
  }
  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }
  stat_file(chp, argv[0]);
}

static void cmd_mkdir(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT res;

  (void)argv;
  if (argc != 1) {
    chprintf(chp, "Usage: mkdir <path>\r\n");
    return;
  }
  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }

  res = f_mkdir(argv[0]);
  if(res != FR_OK) {
	  chprintf(chp, "An error occured. (%d)\n", res);
  }
}

static void cmd_cat(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT res;
  FIL fil;       /* File object */
  char line[82]; /* Line buffer */
  UINT br;

  (void)argv;
  if (argc != 1) {
    chprintf(chp, "Usage: cat <path>\r\n");
    return;
  }
  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }

  /* Open a text file */
  res = f_open(&fil, argv[0], FA_READ);
  if (res != FR_OK) {
	  chprintf(chp, "Could not open file\r\n");
	  return;
  }

  /* Read all lines and display it */
  while (f_read(&fil, line, (sizeof line) - 1, &br) == FR_OK) {
	  if(br == 0) {
		  break;
	  }
	  line[br] = 0;
	  chprintf(chp, line);
  }

  /* Close the file */
  f_close(&fil);
}


static void cmd_i2c(BaseSequentialStream *chp, int argc, char *argv[]) {
	FramebufferSWClear(&fb);

		FramebufferSWSetColor(&fb, COLOR_SW_WHITE);

		//FramebufferSWDrawPixel(&fb, 0, 0);
		//FramebufferSWDrawPixel(&fb, 0, 63);
		//FramebufferSWDrawPixel(&fb, 127, 0);
		//FramebufferSWDrawPixel(&fb, 127, 63);

		FramebufferSWPrintText(&fb, 0, "Title");

		FramebufferSWPrintSMText(&fb, 5, "text", false);

		if (fs_ready) {
			FramebufferSWPrintSMText(&fb, 7, "[X] SD", false);
		} else {
			FramebufferSWPrintSMText(&fb, 7, "[ ] SD", false);
		}

		ssd1306Update(&SSD1306D1);
}

static const ShellCommand commands[] = {
  {"tree", cmd_tree},
  {"stat", cmd_stat},
  {"mkdir", cmd_mkdir},
  {"cat", cmd_cat},
  {"i2c", cmd_i2c},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD2,
  commands
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

#define NUM_SPEED_BUFFER 128
#define MS_PER_INDEX 100

volatile float speedMeasurements[NUM_SPEED_BUFFER] = {0.0};
volatile size_t curSpeedPos = 0;
volatile uint16_t impulses[NUM_SPEED_BUFFER] = {0};
float m_per_impulse = 0.5;
volatile char* fs_stat[50] = {0};

static thread_t *shelltp = NULL;
MMCDriver MMCD1;


/*
 * Card insertion event.
 */
static void InsertHandler(eventid_t id) {
  FRESULT err;

  (void)id;
  /*
   * On insertion SDC initialization and FS mount.
   */
  if (mmcConnect(&MMCD1))
    return;

  err = f_mount(&SDC_FS, "/", 1);
  if (err != FR_OK) {
    mmcDisconnect(&MMCD1);
    return;
  }
  fs_ready = TRUE;
}

/*
 * Card removal event.
 */
static void RemoveHandler(eventid_t id) {

  (void)id;
  mmcDisconnect(&MMCD1);
  fs_ready = FALSE;
}

/*
 * Shell exit event.
 */
static void ShellHandler(eventid_t id) {

  (void)id;
  if (chThdTerminatedX(shelltp)) {
    chThdWait(shelltp);                 /* Returning memory to heap.        */
    shelltp = NULL;
  }
}

/*
 * speed thread.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;

  chRegSetThreadName("speed_update");
  while (TRUE) {
    chThdSleepMilliseconds(MS_PER_INDEX);

    size_t nextPos = (curSpeedPos + 1) % NUM_SPEED_BUFFER;

    speedMeasurements[nextPos] = impulses[nextPos] * m_per_impulse * 1000. / MS_PER_INDEX * 3.6;

    impulses[(nextPos + 1) % NUM_SPEED_BUFFER] = 0;

    curSpeedPos = nextPos;
  }
}


/*
 * Display thread.
 */
static THD_WORKING_AREA(waThread2, 2048);
static THD_FUNCTION(Thread2, arg) {
  (void)arg;

  chRegSetThreadName("display");
   while (TRUE) {
	  	FramebufferSWClear(&fb);

		FramebufferSWSetColor(&fb, COLOR_SW_WHITE);

		char *speedText[20];

		float curSpeed = speedMeasurements[curSpeedPos];

		chsnprintf(speedText, 20, "%d.%d kmh", (int)curSpeed, (int)((curSpeed-(int)curSpeed)*10));

		FramebufferSWPrintText(&fb, 0, speedText);

		FramebufferSWDrawLine(&fb, 0, 52, 127, 52);
		FramebufferSWDrawLine(&fb, 0, 52-32, 127, 52-32);

		FramebufferSWDrawLine(&fb, curSpeedPos, 52-4, curSpeedPos, 52);

		FramebufferSWDrawLine(&fb, curSpeedPos, 52-32, curSpeedPos, 52-28);

		float max = 0.0;

		for(size_t i = 0; i < NUM_SPEED_BUFFER; i++) {
			if(speedMeasurements[i] > max) {
				max = speedMeasurements[i];
			}
		}

		float multiplier = (32. / (max + 8-((int)max % 8)));

		for(size_t i = 0; i < NUM_SPEED_BUFFER; i++) {
			FramebufferSWDrawPixel(&fb, i, 52-(int)(speedMeasurements[i]*multiplier));
		}

		//FramebufferSWDrawLine(&fb, 40, 52, 40, 63);

		FramebufferSWPrintSMText(&fb, 8, fs_stat, false);

		if (fs_ready) {
			//FramebufferSWPrintSMText(&fb, 8, "[X] SD", false);
		} else {
			//FramebufferSWPrintSMText(&fb, 8, "[ ] SD", false);
		}

		ssd1306Update(&SSD1306D1);

		chThdSleepMilliseconds(200);
   }
}

#define FS_DIR "DRC"

/*
 * filesystem thread.
 */
static THD_WORKING_AREA(waThread3, 2048);
static THD_FUNCTION(Thread3, arg) {
  (void)arg;
  FRESULT res;
  static FIL fil;       /* File object */
  static bool fileValid = false;
  static bool wasReady = false;
  static int lastPos = 0;
  static int index = 0;

  UINT bw;

  char* buffer[50];

  chRegSetThreadName("fs");
  while (TRUE) {
    chThdSleepMilliseconds(1000);
    if (!fs_ready) {
    	strcpy(fs_stat, "waiting for SD card");
    	wasReady = false;
    	fileValid = false;
    	continue; // we cannot do anything without a filesystem
    }

    if(!wasReady) {
    	f_mkdir(FS_DIR); // try to create dir
    	wasReady = true;
    	fileValid = false;
    }

    if(!fileValid) {
      /* Open a text file */
      chsnprintf(buffer, 50, "/%s/speed_%d.csv", FS_DIR, 1);
      strcpy(fs_stat, buffer);
	  res = f_open(&fil, buffer, FA_WRITE | FA_OPEN_APPEND);
	  if (res != FR_OK) {
		  continue;
	  }
	  strcpy(buffer, "time;id;ticks;speed\r\n");
	  f_write(&fil, buffer, strlen(buffer), &bw);
	  f_sync(&fil);
	  fileValid = true;
	  index = 0;
    }

    while(lastPos < curSpeedPos || (lastPos > curSpeedPos && curSpeedPos < 10)) {
    	float curSpeed = speedMeasurements[lastPos];
    	chsnprintf(buffer, 50, "%l;%d;%d;%d.%d\r\n", (long)index * MS_PER_INDEX, lastPos, impulses[lastPos], (int)curSpeed, (int)((curSpeed-(int)curSpeed)*10));

    	f_write(&fil, buffer, strlen(buffer), &bw);
    	lastPos = (lastPos + 1) % NUM_SPEED_BUFFER;
    	index ++;
    }

    f_sync(&fil);

  }
}

/* Triggered when the button is pressed or released. The LED is set to ON.*/
static void extcb1(EXTDriver *extp, expchannel_t channel) {

  (void)extp;
  (void)channel;

  impulses[(curSpeedPos + 1) % NUM_SPEED_BUFFER] ++; // TODO: detect if correct edge
  palTogglePad(IOPORT3, GPIOC_LED); // toggle led on impulse
}

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
	{EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};


/*
 * Application entry point.
 */
int main(void) {
  static const evhandler_t evhndl[] = {
    InsertHandler,
    RemoveHandler,
    ShellHandler
  };
  event_listener_t el0, el1, el2;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the EXT driver 1.
   */
  extStart(&EXTD1, &extcfg);
  extChannelEnable(&EXTD1, 0);

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PA9(TX) and PA10(RX) are routed to USART1.
   */
  sdStart(&SD2, NULL);

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * Initializes the MMC driver to work with SPI2.
   */
  palSetPadMode(IOPORT2, GPIOB_SPI2NSS, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(IOPORT2, GPIOB_SPI2NSS);
  mmcObjectInit(&MMCD1);
  mmcStart(&MMCD1, &mmccfg);

  /*
   * Initialize OLED display
   */
  palSetPadMode(GPIOB, 6, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);   /* SCL */
  palSetPadMode(GPIOB, 7, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);   /* SDA */

  FramebufferSWObjectInit(&fb, 128, 64);

  ssd1306ObjectInit(&SSD1306D1);
  ssd1306Start(&SSD1306D1, &ssd1306cfg);

  /*
   * Activates the card insertion monitor.
   */
  tmr_init(&MMCD1);

  /*
   * Creates the speed update thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

  /*
   * Creates the display thread.
   */
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  /*
   * Creates the filesystem thread.
   */
  chThdCreateStatic(waThread3, sizeof(waThread3), NORMALPRIO-1, Thread3, NULL);

  /*
   * Normal main() thread activity, handling SD card events and shell
   * start/exit.
   */
  chEvtRegister(&inserted_event, &el0, 0);
  chEvtRegister(&removed_event, &el1, 1);
  chEvtRegister(&shell_terminated, &el2, 2);
  while (true) {
    if (!shelltp) {
      shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                    "shell", NORMALPRIO + 1,
                                    shellThread, (void *)&shell_cfg1);
    }
    chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
  }
}
