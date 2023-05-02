/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/random/rand32.h>

#define DEFAULT_RADIO_NODE DT_NODELABEL(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay), "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

// LEDS
#define ON  1
#define OFF 0

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec pin_test_led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec pin_test_led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define LED2_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec pin_test_led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define LED3_NODE DT_ALIAS(led3)
static const struct gpio_dt_spec pin_test_led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

#define LED1 &pin_test_led0
#define LED2 &pin_test_led1
#define LED3 &pin_test_led2
#define LED4 &pin_test_led3

#define CON_STATUS_LED LED1
#define RUN_STATUS_LED LED2

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI			{0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x13, 0x44} //LITTLE ENDIAN  msb
#define LORAWAN_JOIN_EUI        {0x60, 0x81, 0xF9, 0x62, 0x41, 0x65, 0x5D, 0x0B}
#define LORAWAN_APP_KEY	        {0x10, 0xF4, 0xCD, 0x51, 0x20, 0x52, 0x7A, 0x9E, 0x14, 0x75, 0x0A, 0xA4, 0x7F, 0x54, 0x46, 0x0B}

#define LORAWAN_DEV_EUI_HELIUM  {0x60, 0x81, 0xF9, 0x07, 0x40, 0x35, 0x0D, 0x69} //msb
#define LORAWAN_JOIN_EUI_HELIUM {0x60, 0x81, 0xF9, 0x82, 0xBD, 0x7F, 0x80, 0xD5} //msb
#define LORAWAN_APP_KEY_HELIUM  {0xE0, 0x07, 0x38, 0x87, 0xAF, 0x4F, 0x16, 0x6E, 0x8E, 0x52, 0xD3, 0x27, 0x0F, 0x2E, 0x64, 0x6F}

#define DELAY K_MSEC(10000)

#define MAX_DATA_LEN 10


#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_node);

char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

static void dl_callback(uint8_t port, bool data_pending,
			int16_t rssi, int8_t snr,
			uint8_t len, const uint8_t *data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
	if (data) {
		LOG_HEXDUMP_INF(data, len, "Payload: ");
	}
}

static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}


void configure_led(void)
{
	gpio_pin_configure_dt(LED1, GPIO_OUTPUT);
	gpio_pin_configure_dt(LED2, GPIO_OUTPUT);
	gpio_pin_configure_dt(LED3, GPIO_OUTPUT);
	gpio_pin_configure_dt(LED4, GPIO_OUTPUT);
}

void turn_off_all_leds(void)
{
	gpio_pin_set_dt(LED1, OFF);
	gpio_pin_set_dt(LED2, OFF);
	gpio_pin_set_dt(LED3, OFF);
	gpio_pin_set_dt(LED4, OFF);
}

void led_on_off(struct gpio_dt_spec led, uint8_t value)
{
	gpio_pin_set_dt(&led, value);
}

void lora_send_func(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DT_NODELABEL(lora0));

	struct lora_modem_config config;
	int ret;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return;
	}

	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 4;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return;
	}

	while (1) {
		ret = lora_send(lora_dev, data, 10);
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			return;
		}

		LOG_INF("Data sent!");

		/* Send data at 1s interval */
		k_sleep(K_MSEC(1000));
	}
}



void main(void)
{
	const struct device *lora_dev;
	struct lorawan_join_config join_cfg;
	

	uint8_t dev_eui[] = LORAWAN_DEV_EUI_HELIUM;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI_HELIUM;
	uint8_t app_key[] = LORAWAN_APP_KEY_HELIUM;

	//uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	//uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	//uint8_t app_key[] = LORAWAN_APP_KEY;



    configure_led();
	turn_off_all_leds();
  
	int ret;

	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};

	//lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0)); // this command is not working
    lora_dev = DEVICE_DT_GET(DT_NODELABEL(lora0));



	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return;
	}

#if defined(CONFIG_LORAMAC_REGION_EU868)
	/* If more than one region Kconfig is selected, app should set region
	 * before calling lorawan_start()
	 */
	ret = lorawan_set_region(LORAWAN_REGION_EU868);
	if (ret < 0) {
		LOG_ERR("lorawan_set_region failed: %d", ret);
		return;
	}
#endif
  





ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
		return;
	}

    k_sleep(K_MSEC(500));//500ms

lorawan_enable_adr( true );

lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);


    uint32_t random = sys_rand32_get();
    uint16_t dev_nonce = random & 0x0000FFFF;

	join_cfg.mode = LORAWAN_CLASS_A;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
    join_cfg.otaa.dev_nonce = dev_nonce;

	
    gpio_pin_set_dt(LED4, OFF);
    ret=-1;
    while(ret<0){
		gpio_pin_set_dt(LED3, OFF);
		gpio_pin_set_dt(LED3, ON);
		gpio_pin_set_dt(LED3, OFF);
		gpio_pin_set_dt(LED3, ON);
		gpio_pin_set_dt(LED3, OFF);

	 LOG_INF("Joining network over OTAA");
	
   do {
    	ret = lorawan_join(&join_cfg);
    	if (ret < 0) {
	    	printk("lorawan_join_network failed: %d\n\n", ret);
            printk("Sleeping for 10s to try again to join network.\n\n");
            k_sleep(K_MSEC(10000));
	    }
    } while ( ret < 0 );




	    gpio_pin_set_dt(LED3, OFF);
		gpio_pin_set_dt(LED3, ON);
		gpio_pin_set_dt(LED3, OFF);
		gpio_pin_set_dt(LED3, ON);
		gpio_pin_set_dt(LED3, OFF);
		gpio_pin_set_dt(LED3, ON);
		gpio_pin_set_dt(LED3, OFF);
	


	  k_sleep(K_MSEC(100));//500ms

     }
    gpio_pin_set_dt(LED4, ON);
	LOG_INF("Sending data...");
	while (1) {
		ret = lorawan_send(2, data, sizeof(data),LORAWAN_MSG_CONFIRMED);

		/*
		 * Note: The stack may return -EAGAIN if the provided data
		 * length exceeds the maximum possible one for the region and
		 * datarate. But since we are just sending the same data here,
		 * we'll just continue.
		 */
		if (ret == -EAGAIN) {
			LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
			k_sleep(DELAY);
			continue;
		}

		if (ret < 0) {
			LOG_ERR("lorawan_send failed: %d", ret);
			return;
		}

		LOG_INF("Data sent!");
		return;// just once
		k_sleep(DELAY);
	}
}
