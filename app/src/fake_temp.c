/* Fake temperature sensor driver -- self-test only (CONFIG_FAKE_TEMP_SENSOR).
 *
 * Returns a slowly-varying temperature (~25 C) through the Zephyr sensor API so
 * the hwmon_i2c -> /sys/class/hwmon/<n>/tempX_input -> HwmonTempSensor dbus
 * path can be exercised without real sensor hardware.
 *
 * Instantiate from a DT overlay (see lsqsh_evb_cpu1_fake_temp.overlay):
 *
 *   &i2c1 {
 *       fake_temp: fake-temp@48 {
 *           compatible = "test,fake-temp-sensor";
 *           reg = <0x48>;
 *           status = "okay";
 *       };
 *   };
 *   / {
 *       hwmon_fake_temp {
 *           compatible = "linkedsemi,hwmon";
 *           class = <0x5>;
 *           label = "hwmon_fake_temp";
 *           sensor-type = "temp";
 *           hwmon-ins = <&fake_temp 0>;
 *           meas = "temp1_input";
 *           status = "okay";
 *       };
 *   };
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

/* DT_DRV_COMPAT must be defined before any DT_INST_* macro so
 * DT_INST_FOREACH_STATUS_OKAY / DEVICE_DT_INST_DEFINE bind to the
 * "test,fake-temp-sensor" instance (the fake_temp@48 node). Without it the
 * FOREACH expands to nothing and no device struct is emitted, which leaves
 * __device_dts_ord_<N> undefined at link time (hwmon_i2c.c references it via
 * DEVICE_DT_GET on the hwmon-ins phandle). */
#define DT_DRV_COMPAT test_fake_temp_sensor

struct fake_temp_data {
	int val_mc; /* milli-degrees Celsius */
};

static int fake_temp_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	struct fake_temp_data *data = dev->data;

	ARG_UNUSED(chan);
	/* 25000..25099 mC, drifts ~1 mC/s so the dbus value visibly changes */
	data->val_mc = 25000 + (int)(k_uptime_get_32() / 1000U) % 100;
	return 0;
}

static int fake_temp_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct fake_temp_data *data = dev->data;

	ARG_UNUSED(chan);
	/* hwmon_i2c requests SENSOR_CHAN_AMBIENT_TEMP for "temp" files and reads
	 * val.val1 as the hwmon ABI value (milli-degrees). Answer any channel
	 * so this stub stays reusable. */
	val->val1 = data->val_mc;
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api fake_temp_api = {
	.sample_fetch = fake_temp_sample_fetch,
	.channel_get = fake_temp_channel_get,
};

static int fake_temp_init(const struct device *dev)
{
	struct fake_temp_data *data = dev->data;

	data->val_mc = 25000;
	return 0;
}

#define FAKE_TEMP_DEFINE(inst)                                              \
	static struct fake_temp_data fake_temp_data_##inst;                \
	DEVICE_DT_INST_DEFINE(inst, fake_temp_init, NULL,                  \
			      &fake_temp_data_##inst, NULL, POST_KERNEL,   \
			      CONFIG_SENSOR_INIT_PRIORITY,                 \
			      &fake_temp_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_TEMP_DEFINE)
