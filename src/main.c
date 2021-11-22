/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <init.h>
#include <inttypes.h>

#include <settings/settings.h>

#include <pm/pm.h>
#include <hal/nrf_gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "hog.h"

/*
 * Get button configuration from the devicetree.
 */
#define BTN_DOWN_NODE	DT_NODELABEL(buttondown)
#define BTN_UP_NODE		DT_NODELABEL(buttonup)
#define BTN_LEFT_NODE	DT_NODELABEL(buttonleft)
#define BTN_RIGHT_NODE	DT_NODELABEL(buttonright)
#define BTN_ENTER_NODE	DT_NODELABEL(buttonenter)
#define BTN_HOME_NODE	DT_NODELABEL(buttonhome)
#define BTN_BACK_NODE	DT_NODELABEL(buttonback)
#define BTN_PAUSE_NODE	DT_NODELABEL(buttonpause)
#define BTN_EXTRA_NODE	DT_NODELABEL(buttonextra)

#define BTN_PORT_NODE DT_NODELABEL(gpio0)

static const struct gpio_dt_spec button_down = GPIO_DT_SPEC_GET_OR(BTN_DOWN_NODE, gpios, {0});
static const struct gpio_dt_spec button_up = GPIO_DT_SPEC_GET_OR(BTN_UP_NODE, gpios, {0});
static const struct gpio_dt_spec button_left = GPIO_DT_SPEC_GET_OR(BTN_LEFT_NODE, gpios, {0});
static const struct gpio_dt_spec button_right = GPIO_DT_SPEC_GET_OR(BTN_RIGHT_NODE, gpios, {0});
static const struct gpio_dt_spec button_enter = GPIO_DT_SPEC_GET_OR(BTN_ENTER_NODE, gpios, {0});
static const struct gpio_dt_spec button_home = GPIO_DT_SPEC_GET_OR(BTN_HOME_NODE, gpios, {0});
static const struct gpio_dt_spec button_back = GPIO_DT_SPEC_GET_OR(BTN_BACK_NODE, gpios, {0});
static const struct gpio_dt_spec button_pause = GPIO_DT_SPEC_GET_OR(BTN_PAUSE_NODE, gpios, {0});
static const struct gpio_dt_spec button_extra = GPIO_DT_SPEC_GET_OR(BTN_EXTRA_NODE, gpios, {0});

static const struct device* button_port = DEVICE_DT_GET(BTN_PORT_NODE);

/*
 * Map buttons and keys.
 */
static const int KEY_F1 = 0x3a;   		/* F1 key HOME */
static const int KEY_F2 = 0x3b;         /* F2 key BACK */
static const int KEY_F4 = 0x3d;         /* F4 key MENU */
static const int KEY_F5 = 0x3e;         /* F5 key POWEROFF */
 
static const int RIGHT_ARROW = 0x4f;    /* Right arrow */
static const int LEFT_ARROW = 0x50;     /* Left arrow */
static const int DOWN_ARROW = 0x51;     /* Down arrow */
static const int UP_ARROW = 0x52;       /* Up arrow */
static const int ENTER = 0x28;			/* Enter */

static const int buttonlistSize = 9;
static const struct gpio_dt_spec* buttonlist[9] = {&button_down, &button_up, &button_left, &button_right, &button_enter, &button_home, &button_back, &button_pause, &button_extra};
static const int buttonKeyMapping[9] = { DOWN_ARROW, UP_ARROW, LEFT_ARROW, RIGHT_ARROW, ENTER, KEY_F1, KEY_F2, KEY_F4, KEY_F5 };

static struct gpio_callback button_cb_data;

static const int hogDataSize = 8;
static volatile uint8_t hogData[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static volatile bool hogDataUpdated;

// Defines how long the 
static uint32_t resetTiming;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	// Reset previous state.
	memset((void*) hogData, 0, sizeof(hogData));
	int hogIdx = 2;
	int btnIdx = 0;
	while(btnIdx < buttonlistSize && hogIdx < hogDataSize) {
		if(gpio_pin_get_dt(buttonlist[btnIdx]) == 1) {
			hogData[hogIdx++] = buttonKeyMapping[btnIdx];
		}
		btnIdx++;
	}
	hogDataUpdated = true;
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0xC1, 0x03),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		return;
	}

	if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	if (!err) {
		// ok
	} else {
		// oh no.
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void bt_ready(int err)
{
	if (err) {
		return;
	}

	hog_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		return;
	}
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	bt_conn_auth_passkey_confirm(conn);
}

static void auth_passkey_entry(struct bt_conn *conn) {
	bt_conn_auth_passkey_entry(conn, 0);
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
	bt_conn_auth_pairing_confirm(conn);
}

static void auth_cancel(struct bt_conn *conn)
{
}

/*static void auth_pairing_complete(struct bt_conn *conn, bool bonded) {
	if(bonded) {
	}
}*/

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.passkey_entry = auth_passkey_entry, // Bonding with this will fail.
	.pairing_confirm = auth_pairing_confirm,
	//.pairing_complete = auth_pairing_complete,
	.cancel = auth_cancel,
};

void main(void)
{
	int err;
	
	if (!device_is_ready(button_port)) {
		return;
	}

	// Zephyr button setup: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/basic/button
	for(int i = 0; i < buttonlistSize; i++) {
		err = gpio_pin_configure_dt(buttonlist[i], GPIO_INPUT);
		err = gpio_pin_interrupt_configure_dt(buttonlist[i], GPIO_INT_EDGE_BOTH);
	}

	gpio_init_callback(&button_cb_data, button_pressed, 
		BIT(button_down.pin) | 
		BIT(button_up.pin) | 
		BIT(button_left.pin) | 
		BIT(button_right.pin) | 
		BIT(button_enter.pin) | 
		BIT(button_home.pin) |
		BIT(button_back.pin) |
		BIT(button_pause.pin) |
		BIT(button_extra.pin)
	);
	// All use the same port. (idk how this is defined)
	gpio_add_callback(button_port, &button_cb_data);

	err = bt_enable(bt_ready);
	if (err) {
		return;
	}

	err = bt_passkey_set(0);
	if (err) {
		return;
	}
	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	uint32_t nextSleepTime = k_cycle_get_32()/sys_clock_hw_cycles_per_sec()+300; // Next system off in 5min.
	while(true) {
		// Read the button press and send it.
		if(hogDataUpdated) {
			nextSleepTime = k_cycle_get_32()/sys_clock_hw_cycles_per_sec()+300; // Reset next system off.
			hogDataUpdated = false;

			// Send data to any connected device. (adv is stopped after 1st connection)
			hog_send_button(NULL, (uint8_t*)hogData, sizeof(hogData));

			// Reset combination pressed.
			if(hogData[2] == buttonKeyMapping[7] && hogData[3] == buttonKeyMapping[8])
				resetTiming = k_cycle_get_32()/sys_clock_hw_cycles_per_sec() + 5;
			else
				resetTiming = 0;
		}

		// Reset still pressed and cycles are over. Unpair all devices.
		if(resetTiming != 0 && resetTiming < k_cycle_get_32()/sys_clock_hw_cycles_per_sec()) {
			resetTiming = 0;
			bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
		}
		if(nextSleepTime < k_cycle_get_32()/sys_clock_hw_cycles_per_sec()) {
			// NOTE: If hw enters the SOFT_OFF mode, nrf_power_system_off() is called
			// The program won't be possible to continue and will start from the beginning.

			// We set the buttons using the NRF SENSE event.
			// See: https://devzone.nordicsemi.com/f/nordic-q-a/19859/system-off-and-gpiote
			for(int i = 0; i < buttonlistSize; i++) {		
				nrf_gpio_cfg_input(buttonlist[i]->pin, NRF_GPIO_PIN_PULLDOWN);
				nrf_gpio_cfg_sense_set(buttonlist[i]->pin, NRF_GPIO_PIN_SENSE_HIGH);
			}

			// We turn off the advertisement so zephyr can suspend the BLE thread.
			// Probably not required.
			err = bt_le_adv_stop();
			// Set SOFT_OFF as desired state.
			// Runs pm_power_state_set which calls nrf_power_system_off(NRF_POWER)
			pm_power_state_force((struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0, 0});
		} else {
			k_sleep(K_MSEC(250));
		}
	}
}
