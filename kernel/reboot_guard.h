#ifndef __KSU_H_REBOOT_GUARD
#define __KSU_H_REBOOT_GUARD

#include <linux/types.h>

bool ksu_reboot_guard_armed(void);
void ksu_reboot_guard_set(bool armed);

bool ksu_should_intercept_reboot(int cmd, pid_t caller_pid);
void ksu_real_reboot_once(void);

bool ksu_soft_reboot_in_progress(void);
void ksu_set_soft_reboot_in_progress(bool val);

void ksu_reboot_guard_init(void);
void ksu_reboot_guard_exit(void);

#endif /* __KSU_H_REBOOT_GUARD */
