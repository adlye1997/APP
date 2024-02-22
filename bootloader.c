#include "bootloader.h"
#include "stm32f103_bsp_flash.h"

// Flash分区
#define BOOTLOADER_ADDRESS  0x08000000
#define SETTING_ADDRESS     0x08003C00
#define APPLICATION_ADDRESS 0x08004000
#define DOWNLOAD_ADDRESS    0x08008000

// Setting标志
#define BOOTFLAG 0x12345678

Boot boot;
extern Xmodem xmodem_boot;

static void BootReceiveCallback(uint8_t *rxbuf, uint16_t len) {
	BootUpdate(&boot);
	GetFirmwareToDownload(rxbuf, len);
}

void BootSetup(void) {
	BootInit(&boot, &xmodem_boot, 5000, BootReceiveCallback);

	// 初始化向量表偏移量
	SCB->VTOR = BOOTLOADER_ADDRESS;

	// 读取是否bootloader标志位
	uint32_t data = 0;
	ReadFlashU32(&data, 1, SETTING_ADDRESS);

	// 启动bootloader
	if(data == BOOTFLAG)
	{
		//清除下载缓冲区
		EraseFlashPage(DOWNLOAD_ADDRESS, 16);

		//清除标志位
		data = 0xFFFFFFFF;
		OverWriteFlashU32(&data, 1, SETTING_ADDRESS);

		// 初始化完成
		boot.status = BOOT_DOWNLOADING;

		// 发送请求
		boot.protocol->Begin();
	}
	// 不启动bootloader
	else
	{
		//清除标志位
		EraseFlashPage(SETTING_ADDRESS, 1);

		//跳转应用程序
		JumpToApplication();
	}
}

void BootLoop(void) {
	BootPolling(&boot);
}

static void BootInit(Boot *boot, Xmodem *protocol, uint16_t tick_timeout, \
			void ReceiveCallback(uint8_t*,uint16_t)) {
	boot->status = BOOT_INIT;
	boot->tick_update = HAL_GetTick();
	boot->tick_timeout = tick_timeout;
	boot->protocol = protocol;
	boot->ReceiveCallback = ReceiveCallback;
	boot->downloaded_num = 0;
}

static void BootUpdate(Boot *boot) {
	boot->tick_update = HAL_GetTick();
}

static void BootPolling(Boot *boot) {
	if(boot->status == BOOT_DOWNLOADING) {
		if(HAL_GetTick() - boot->tick_update >= boot->tick_timeout) {
			boot->status = BOOT_TIMEOUT;
		}
		if(boot->protocol->status == XMODEM_END) {
			boot->status = BOOT_UPDATING;
		}
	}
	if(boot->status == BOOT_UPDATING) {
		DownloadUpdateToApp();
		JumpToApplication();
	}
	if(boot->status == BOOT_TIMEOUT) {
		JumpToApplication();
	}
}

static void JumpToApplication(void)
{
	void (*Func)(void);
	uint32_t JumpAddress;

	// 关闭所有中断
	__disable_irq();

	// 重置所有外设，初始化为默认状态
	HAL_DeInit();

	// 设置向量表偏移量为应用程序的起始地址
	SCB->VTOR = APPLICATION_ADDRESS;

	// 设置主堆栈指针（MSP）
	__set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);

	// 获取复位向量地址
	JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
	Func = (void (*)(void)) JumpAddress;

	// 跳转到应用程序
	Func();
}

static void GetFirmwareToDownload(uint8_t *data, uint16_t length) {
	WriteFlashU32((uint32_t*)data, length/4, DOWNLOAD_ADDRESS+boot.downloaded_num);
	boot.downloaded_num += length;
}

static void DownloadUpdateToApp(void) {
	uint32_t data;
	EraseFlashPage(0x08004000, 16);
	for(uint16_t i = 0; i < boot.downloaded_num / 4; i++) {
		ReadFlashU32(&data, 1, DOWNLOAD_ADDRESS + i * 4);
		WriteFlashU32(&data, 1, APPLICATION_ADDRESS + i * 4);
	}
}
