#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f1xx_hal.h"
#include "protocol_xmodem.h"

/*
 * Flash空间
 * c8t6: 1K * 64
*/

typedef struct {
	uint8_t status;
	uint16_t tick_update;
	uint16_t tick_timeout;
	uint32_t downloaded_num;
	Xmodem *protocol;
	void (*ReceiveCallback)(uint8_t *a, uint16_t b);
}Boot;

typedef enum {
	BOOT_INIT,
	BOOT_DOWNLOADING,
	BOOT_UPDATING,
	BOOT_TIMEOUT,
	BOOT_ERROR,
	BOOT_DONE
}BootStatus;

void BootSetup(void);
void BootLoop(void);

static void BootInit(Boot *boot, Xmodem *protocol, uint16_t tick_timeout, \
			void ReceiveCallback(uint8_t*,uint16_t));
static void BootUpdate(Boot *boot);
static void BootPolling(Boot *boot);
static void JumpToApplication(void);
static void GetFirmwareToDownload(uint8_t *data, uint16_t length);
static void DownloadUpdateToApp(void);

#endif // !__BOOTLOADER_H

