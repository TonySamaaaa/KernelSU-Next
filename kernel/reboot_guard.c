/*
 * Reboot Guard: intercept hard reboot(2) and convert it into a soft reboot
 * to preserve temp-root/jailbreak sessions.
 *
 * Layer 3 only: reboot(2) backstop inside the existing reboot kprobe.
 * The guard is armed automatically in late-load mode (no Manager feature),
 * so the Manager app is completely unaware of it.
 *
 * Never intercepts:
 * - PID 1's reboot(2) (init already completed teardown; must pass through)
 * - shutdown/recovery/bootloader/fastboot/download/edl commands
 *
 * Escape hatch: KSU_IOCTL_REAL_REBOOT_ONCE lets exactly one reboot pass
 * through (recovery/OTA/forced-restart cases).
 */

#include <linux/atomic.h>
#include <linux/printk.h>
#include <linux/reboot.h>
#include <linux/types.h>

#include "reboot_guard.h"

static atomic_t guard_armed = ATOMIC_INIT(0);
static atomic_t soft_reboot_running = ATOMIC_INIT(0);
static atomic_t real_reboot_once_flag = ATOMIC_INIT(0);

bool ksu_reboot_guard_armed(void)
{
	return atomic_read(&guard_armed) != 0;
}

void ksu_reboot_guard_set(bool armed)
{
	atomic_set(&guard_armed, armed ? 1 : 0);
	pr_info("ksu: reboot guard %s\n", armed ? "armed" : "disarmed");
}

bool ksu_soft_reboot_in_progress(void)
{
	return atomic_read(&soft_reboot_running) != 0;
}

void ksu_set_soft_reboot_in_progress(bool val)
{
	atomic_set(&soft_reboot_running, val ? 1 : 0);
}

/*
 * Layer 3: should this reboot(2) call be intercepted?
 *
 * Rules:
 * - guard must be armed
 * - one-shot escape hatch lets exactly one reboot pass through
 * - PID 1 always passes through
 * - only RESTART/RESTART2 are intercepted
 */
bool ksu_should_intercept_reboot(int cmd, pid_t caller_pid)
{
	if (!ksu_reboot_guard_armed())
		return false;

	/* One-shot escape hatch: let exactly one reboot through */
	if (atomic_cmpxchg(&real_reboot_once_flag, 1, 0) == 1) {
		pr_info("ksu: reboot guard: one-shot pass-through (REAL_REBOOT_ONCE)\n");
		return false;
	}

	/* PID 1 must always pass through */
	if (caller_pid == 1) {
		pr_info("ksu: reboot guard: PID 1 reboot, passing through\n");
		return false;
	}

	switch (cmd) {
	case LINUX_REBOOT_CMD_RESTART:
	case LINUX_REBOOT_CMD_RESTART2:
		if (ksu_soft_reboot_in_progress()) {
			pr_info("ksu: reboot guard: soft reboot already in progress, ignoring\n");
			return true;
		}
		pr_info("ksu: reboot guard: intercepting reboot from PID %d, kicking soft reboot\n",
			caller_pid);
		return true;

	default:
		/* halt, power_off, kexec, etc. pass through */
		return false;
	}
}

void ksu_real_reboot_once(void)
{
	atomic_set(&real_reboot_once_flag, 1);
	pr_info("ksu: reboot guard: armed one-shot pass-through\n");
}

void ksu_reboot_guard_init(void)
{
	pr_info("ksu: reboot guard initialized (auto-armed in late-load mode)\n");
}

void ksu_reboot_guard_exit(void)
{
	pr_info("ksu: reboot guard exit\n");
}
