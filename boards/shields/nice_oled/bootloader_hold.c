#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>

#define BOOTLOADER_LAYER 4
#define BOOTLOADER_POSITION 2

static bool key_held;

static void enter_bootloader(struct k_work *work) {
    if (!key_held || zmk_keymap_highest_layer_active() != BOOTLOADER_LAYER) {
        return;
    }

    const struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(bootloader)),
    };
    const struct zmk_behavior_binding_event event = {
        .layer = BOOTLOADER_LAYER,
        .position = BOOTLOADER_POSITION,
        .timestamp = k_uptime_get(),
    };

    zmk_behavior_invoke_binding(&binding, event, true);
}

K_WORK_DELAYABLE_DEFINE(bootloader_hold, enter_bootloader);

static int bootloader_hold_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);

    if (event == NULL || event->position != BOOTLOADER_POSITION) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (event->state && zmk_keymap_highest_layer_active() == BOOTLOADER_LAYER) {
        key_held = true;
        k_work_reschedule(&bootloader_hold, K_SECONDS(3));
    } else {
        key_held = false;
        k_work_cancel_delayable(&bootloader_hold);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(bootloader_hold, bootloader_hold_listener);
ZMK_SUBSCRIPTION(bootloader_hold, zmk_position_state_changed);
