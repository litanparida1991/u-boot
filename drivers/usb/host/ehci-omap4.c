/*
 * OMAP4 EHCI port, copied from linux/drivers/usb/host/ehci-omap.c
 *
 * Copyright (C) 2007-2010 Texas Instruments, Inc.
 *	Author: Vikram Pandita <vikram.pandita@ti.com>
 *	Author: Anand Gadiyar <gadiyar@ti.com>
 */

#include <common.h>
#include <usb.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/omap4.h>
#include <asm/arch/sys_proto.h>

#include "ehci.h"
#include "ehci-core.h"

#define EHCI_BASE (OMAP44XX_L4_CORE_BASE + 0x64C00)
#define UHH_BASE (OMAP44XX_L4_CORE_BASE + 0x64000)
#define TLL_BASE (OMAP44XX_L4_CORE_BASE + 0x62000)

/* ULPI */
#define ULPI_SET(a)				(a + 1)
#define ULPI_CLR(a)				(a + 2)

#define ULPI_FUNC_CTRL				0x04

#define ULPI_FUNC_CTRL_RESET			(1 << 5)

/* TLL Register Set */
#define	OMAP_USBTLL_REVISION				(0x00)
#define	OMAP_USBTLL_SYSCONFIG				(0x10)
#define	OMAP_USBTLL_SYSCONFIG_CACTIVITY			(1 << 8)
#define	OMAP_USBTLL_SYSCONFIG_SIDLEMODE			(1 << 3)
#define	OMAP_USBTLL_SYSCONFIG_ENAWAKEUP			(1 << 2)
#define	OMAP_USBTLL_SYSCONFIG_SOFTRESET			(1 << 1)
#define	OMAP_USBTLL_SYSCONFIG_AUTOIDLE			(1 << 0)

#define	OMAP_USBTLL_SYSSTATUS				(0x14)
#define	OMAP_USBTLL_SYSSTATUS_RESETDONE			(1 << 0)

#define	OMAP_USBTLL_IRQSTATUS				(0x18)
#define	OMAP_USBTLL_IRQENABLE				(0x1C)

#define	OMAP_TLL_SHARED_CONF				(0x30)
#define	OMAP_TLL_SHARED_CONF_USB_90D_DDR_EN		(1 << 6)
#define	OMAP_TLL_SHARED_CONF_USB_180D_SDR_EN		(1 << 5)
#define	OMAP_TLL_SHARED_CONF_USB_DIVRATION		(1 << 2)
#define	OMAP_TLL_SHARED_CONF_FCLK_REQ			(1 << 1)
#define	OMAP_TLL_SHARED_CONF_FCLK_IS_ON			(1 << 0)

#define	OMAP_TLL_CHANNEL_CONF(num)			(0x040 + 0x004 * num)
#define	OMAP_TLL_CHANNEL_CONF_ULPINOBITSTUFF		(1 << 11)
#define	OMAP_TLL_CHANNEL_CONF_ULPI_ULPIAUTOIDLE		(1 << 10)
#define	OMAP_TLL_CHANNEL_CONF_UTMIAUTOIDLE		(1 << 9)
#define	OMAP_TLL_CHANNEL_CONF_ULPIDDRMODE		(1 << 8)
#define	OMAP_TLL_CHANNEL_CONF_CHANEN			(1 << 0)

#define	OMAP_TLL_ULPI_FUNCTION_CTRL(num)		(0x804 + 0x100 * num)
#define	OMAP_TLL_ULPI_INTERFACE_CTRL(num)		(0x807 + 0x100 * num)
#define	OMAP_TLL_ULPI_OTG_CTRL(num)			(0x80A + 0x100 * num)
#define	OMAP_TLL_ULPI_INT_EN_RISE(num)			(0x80D + 0x100 * num)
#define	OMAP_TLL_ULPI_INT_EN_FALL(num)			(0x810 + 0x100 * num)
#define	OMAP_TLL_ULPI_INT_STATUS(num)			(0x813 + 0x100 * num)
#define	OMAP_TLL_ULPI_INT_LATCH(num)			(0x814 + 0x100 * num)
#define	OMAP_TLL_ULPI_DEBUG(num)			(0x815 + 0x100 * num)
#define	OMAP_TLL_ULPI_SCRATCH_REGISTER(num)		(0x816 + 0x100 * num)

#define OMAP_TLL_CHANNEL_COUNT				3
#define OMAP_TLL_CHANNEL_1_EN_MASK			(1 << 1)
#define OMAP_TLL_CHANNEL_2_EN_MASK			(1 << 2)
#define OMAP_TLL_CHANNEL_3_EN_MASK			(1 << 4)

/* UHH Register Set */
#define	OMAP_UHH_REVISION				(0x00)
#define	OMAP_UHH_SYSCONFIG				(0x10)
#define	OMAP_UHH_SYSCONFIG_MIDLEMODE			(1 << 12)
#define	OMAP_UHH_SYSCONFIG_CACTIVITY			(1 << 8)
#define	OMAP_UHH_SYSCONFIG_SIDLEMODE			(1 << 3)
#define	OMAP_UHH_SYSCONFIG_ENAWAKEUP			(1 << 2)
#define	OMAP_UHH_SYSCONFIG_SOFTRESET			(1 << 1)
#define	OMAP_UHH_SYSCONFIG_AUTOIDLE			(1 << 0)

#define	OMAP_UHH_SYSSTATUS				(0x14)
#define	OMAP_UHH_HOSTCONFIG				(0x40)
#define	OMAP_UHH_HOSTCONFIG_ULPI_BYPASS			(1 << 0)
#define	OMAP_UHH_HOSTCONFIG_ULPI_P1_BYPASS		(1 << 0)
#define	OMAP_UHH_HOSTCONFIG_ULPI_P2_BYPASS		(1 << 11)
#define	OMAP_UHH_HOSTCONFIG_ULPI_P3_BYPASS		(1 << 12)
#define OMAP_UHH_HOSTCONFIG_INCR4_BURST_EN		(1 << 2)
#define OMAP_UHH_HOSTCONFIG_INCR8_BURST_EN		(1 << 3)
#define OMAP_UHH_HOSTCONFIG_INCR16_BURST_EN		(1 << 4)
#define OMAP_UHH_HOSTCONFIG_INCRX_ALIGN_EN		(1 << 5)
#define OMAP_UHH_HOSTCONFIG_P1_CONNECT_STATUS		(1 << 8)
#define OMAP_UHH_HOSTCONFIG_P2_CONNECT_STATUS		(1 << 9)
#define OMAP_UHH_HOSTCONFIG_P3_CONNECT_STATUS		(1 << 10)

/* OMAP4-specific defines */
#define OMAP4_UHH_SYSCONFIG_IDLEMODE_CLEAR		(3 << 2)
#define OMAP4_UHH_SYSCONFIG_NOIDLE			(1 << 2)

#define OMAP4_UHH_SYSCONFIG_STDBYMODE_CLEAR		(3 << 4)
#define OMAP4_UHH_SYSCONFIG_NOSTDBY			(1 << 4)
#define OMAP4_UHH_SYSCONFIG_SOFTRESET			(1 << 0)

#define OMAP4_P1_MODE_CLEAR				(3 << 16)
#define OMAP4_P1_MODE_TLL				(1 << 16)
#define OMAP4_P1_MODE_HSIC				(3 << 16)
#define OMAP4_P2_MODE_CLEAR				(3 << 18)
#define OMAP4_P2_MODE_TLL				(1 << 18)
#define OMAP4_P2_MODE_HSIC				(3 << 18)

#define OMAP_REV2_TLL_CHANNEL_COUNT			2

#define	OMAP_UHH_DEBUG_CSR				(0x44)

/* EHCI Register Set */
#define EHCI_INSNREG04					(0xA0)
#define EHCI_INSNREG04_DISABLE_UNSUSPEND		(1 << 5)
#define	EHCI_INSNREG05_ULPI				(0xA4)
#define	EHCI_INSNREG05_ULPI_CONTROL_SHIFT		31
#define	EHCI_INSNREG05_ULPI_PORTSEL_SHIFT		24
#define	EHCI_INSNREG05_ULPI_OPSEL_SHIFT			22
#define	EHCI_INSNREG05_ULPI_REGADD_SHIFT		16
#define	EHCI_INSNREG05_ULPI_EXTREGADD_SHIFT		8
#define	EHCI_INSNREG05_ULPI_WRDATA_SHIFT		0

int omap4_ehci_hcd_init(void)
{
	unsigned long base = get_timer(0);
	unsigned reg = 0, port = 0;
	int rc;

	/* USB host, with clock from external phy as port 1 UTMI clock */
	sr32((void *)0x4A009358, 0, 32, 0x01000002);

	/* FSUSB clk */
	sr32((void *)0x4a0093d0, 0, 32, 0x2);

	/* USB TLL clock */
	sr32((void *)0x4a009368, 0, 32, 0x1);

	/* enable the 32K, 48M optional clocks and enable the module */
	sr32((void *)0x4a0093e0, 0, 32, 0x301);

	/* perform TLL soft reset, and wait until reset is complete */
	writel(OMAP_USBTLL_SYSCONFIG_SOFTRESET,
	       TLL_BASE + OMAP_USBTLL_SYSCONFIG);

	/* Wait for TLL reset to complete */
	while (!(readl(TLL_BASE + OMAP_USBTLL_SYSSTATUS)
		 & OMAP_USBTLL_SYSSTATUS_RESETDONE))
		if (get_timer(base) > CONFIG_SYS_HZ) {
			printf("OMAP4 EHCI error: timeout resetting TLL\n");
			return -1;
		}

	writel(OMAP_USBTLL_SYSCONFIG_ENAWAKEUP |
	       OMAP_USBTLL_SYSCONFIG_SIDLEMODE |
	       OMAP_USBTLL_SYSCONFIG_CACTIVITY,
	       TLL_BASE + OMAP_USBTLL_SYSCONFIG);

	/* Put UHH in NoIdle/NoStandby mode */
	reg = readl(UHH_BASE + OMAP_UHH_SYSCONFIG);
	reg &= ~OMAP4_UHH_SYSCONFIG_IDLEMODE_CLEAR;
	reg |= OMAP4_UHH_SYSCONFIG_NOIDLE;
	reg &= ~OMAP4_UHH_SYSCONFIG_STDBYMODE_CLEAR;
	reg |= OMAP4_UHH_SYSCONFIG_NOSTDBY;
	writel(reg, UHH_BASE + OMAP_UHH_SYSCONFIG);

	reg = readl(UHH_BASE + OMAP_UHH_HOSTCONFIG);

	/* setup ULPI bypass and burst configurations */
	reg |= (OMAP_UHH_HOSTCONFIG_INCR4_BURST_EN
			| OMAP_UHH_HOSTCONFIG_INCR8_BURST_EN
			| OMAP_UHH_HOSTCONFIG_INCR16_BURST_EN);
	reg &= ~OMAP_UHH_HOSTCONFIG_INCRX_ALIGN_EN;

	/* Clear port mode fields for PHY mode*/
	reg &= ~OMAP4_P1_MODE_CLEAR;
	reg &= ~OMAP4_P2_MODE_CLEAR;
	writel(reg, UHH_BASE + OMAP_UHH_HOSTCONFIG);

	/*
	 * An undocumented "feature" in the OMAP3 EHCI controller,
	 * causes suspended ports to be taken out of suspend when
	 * the USBCMD.Run/Stop bit is cleared (for example when
	 * we do ehci_bus_suspend).
	 * This breaks suspend-resume if the root-hub is allowed
	 * to suspend. Writing 1 to this undocumented register bit
	 * disables this feature and restores normal behavior.
	 */
	writel(EHCI_INSNREG04_DISABLE_UNSUSPEND, EHCI_BASE + EHCI_INSNREG04);

	reg = ULPI_FUNC_CTRL_RESET
		/* FUNCTION_CTRL_SET register */
		| (ULPI_SET(ULPI_FUNC_CTRL) << EHCI_INSNREG05_ULPI_REGADD_SHIFT)
		/* Write */
		| (2 << EHCI_INSNREG05_ULPI_OPSEL_SHIFT)
		/* PORTn */
		| ((port + 1) << EHCI_INSNREG05_ULPI_PORTSEL_SHIFT)
		/* start ULPI access*/
		| (1 << EHCI_INSNREG05_ULPI_CONTROL_SHIFT);

	base = get_timer(0);

	writel(reg, EHCI_BASE + EHCI_INSNREG05_ULPI);

	/* Wait for ULPI access completion */
	while ((readl(EHCI_BASE + EHCI_INSNREG05_ULPI)
		& (1 << EHCI_INSNREG05_ULPI_CONTROL_SHIFT)))
		if (get_timer(base) > CONFIG_SYS_HZ) {
			printf("OMAP4 EHCI error: timeout resetting phy\n");
			return -1;
		}

	hccr = (struct ehci_hccr *)(EHCI_BASE);
	hcor = (struct ehci_hcor *)((uint32_t) hccr
			+ HC_LENGTH(ehci_readl(&hccr->cr_capbase)));
	return 0;
}

int omap4_ehci_hcd_stop(void)
{
	unsigned base = get_timer(0);

	writel(OMAP4_UHH_SYSCONFIG_SOFTRESET, UHH_BASE + OMAP_UHH_SYSCONFIG);

#if 0
	/* We get timeout here */
	while (!(readl(UHH_BASE + OMAP_UHH_SYSSTATUS) & (1 << 0)))
		if (get_timer(base) > CONFIG_SYS_HZ) {
			printf("OMAP4 EHCI error: reset UHH 0 timeout\n");
			return -ETIMEDOUT;
		}

	while (!(readl(UHH_BASE + OMAP_UHH_SYSSTATUS) & (1 << 1)))
		if (get_timer(base) > CONFIG_SYS_HZ) {
			printf("OMAP4 EHCI error: reset UHH 1 timeout\n");
			return -ETIMEDOUT;
		}

	while (!(readl(UHH_BASE + OMAP_UHH_SYSSTATUS) & (1 << 2)))
		if (get_timer(base) > CONFIG_SYS_HZ) {
			printf("OMAP4 EHCI error: reset UHH 2 timeout\n");
			return -ETIMEDOUT;
		}
#endif


	writel((1 << 1), TLL_BASE + OMAP_USBTLL_SYSCONFIG);

	while (!(readl(TLL_BASE + OMAP_USBTLL_SYSSTATUS) & (1 << 0)))
		if (get_timer(base) > CONFIG_SYS_HZ) {
			printf("OMAP4 EHCI error: reset TLL timeout\n");
			return -ETIMEDOUT;
		}

	/* Disable clocks */
	sr32((void *)0x4a0093e0, 0, 32, 0);
	sr32((void *)0x4a009368, 0, 32, 0);
	sr32((void *)0x4a0093d0, 0, 32, 0);
	sr32((void *)0x4A009358, 0, 32, 0);

	return 0;
}
