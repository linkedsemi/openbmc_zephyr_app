/* Fake PSU sensor driver -- self-test only (CONFIG_FAKE_PSU_SENSOR).
 *
 * Returns slowly-varying voltage/power rail values through the Zephyr sensor
 * API so the hwmon_i2c -> sysfs <rail>_input -> PSUSensor dbus path can be
 * exercised without real PMBus hardware.
 *
 * Instantiate from a DT overlay (see lsqsh_evb_cpu1_fake_psu.overlay):
 *
 *   &i2c1 {
 *       fake_psu: fake-psu@4a {
 *           compatible = "test,fake-psu-sensor";
 *           reg = <0x4a>;
 *           #hwmon-in-cells = <1>;
 *           status = "okay";
 *       };
 *   };
 *   / {
 *       hwmon_fake_psu {
 *           compatible = "linkedsemi,hwmon";
 *           class = <0x5>;
 *           label = "hwmon_fake_psu";
 *           sensor-type = "psu";
 *           hwmon-ins = <&fake_psu 0>;
 *           meas = "in1_input:vin", "in3_input:vout1",
 *                  "power2_input:pin1", "power3_input:pout1";
 *           status = "okay";
 *       };
 *   };
 *
 * hwmon_i2c requests one of SENSOR_CHAN_VOLTAGE / POWER / CURRENT /
 * AMBIENT_TEMP / RPM (depending on the "in*"/"power*"/"curr*"/"temp*"/"fan*"
 * file prefix) and passes (file index - 1) as val2 -- the rail / sub-channel
 * selector. hwmon_i2c prints val1 directly (no scaling), and the pmbus
 * convention used by PSUSensor is (raw value -> dbus, divided by 10^Scale):
 *   in*_input    -> millivolts      (Scale 3: /1000)  -> V
 *   power*_input -> microwatts      (Scale 6: /10^6)  -> W
 *   curr*_input  -> milliamps       (Scale 3: /1000)  -> A
 *   temp*_input  -> millidegrees C  (Scale 3: /1000)  -> degC
 *   fan*_input   -> RPM             (Scale 0: /1)     -> RPM
 * So this stub must return hwmon-standard raw values (mV / uW / mA / mC / RPM),
 * calibrated to yield sensible dbus readings, with a slow drift so the values
 * visibly change.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT test_fake_psu_sensor

struct fake_psu_data {
	int tick; /* 0..99, drifts ~1/s */
};

static int fake_psu_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct fake_psu_data *data = dev->data;

	ARG_UNUSED(chan);
	data->tick = (int)(k_uptime_get_32() / 1000U) % 100;
	return 0;
}

static int fake_psu_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct fake_psu_data *data = dev->data;
	/* hwmon_i2c pre-sets val2 = (file index - 1) as the rail selector:
	 *   in1_input   -> 0 (vin),  in2_input   -> 1 (vin1), in3_input -> 2 (vout1)
	 *   power2_input-> 1 (pin1), power3_input-> 2 (pout1)
	 *   curr1_input -> 0 (iout1), temp1_input -> 0 (temp1), fan1_input -> 0 (fan1) */
	int rail = (int)val->val2;
	/* Center drift at 0 so readings vary slowly around the nominal value. */
	int drift = data->tick - 50;

	ARG_UNUSED(dev);

	/* hwmon_i2c prints val1 directly; PSUSensor applies the pmbus Scale
	 * (voltage: /1000 mV->V, power: /10^6 uW->W, curr: /1000 mA->A,
	 * temp: /1000 mC->C, fan: /1 RPM). Return raw hwmon-standard values so
	 * dbus shows ~12V/3.3V, ~12W/3W, ~1A, ~35C, ~5000RPM. */
	switch (chan) {
		case SENSOR_CHAN_VOLTAGE:
			/* rail 0 -> vin (12V), rail 1 -> vin1 (12V),
			 * rail 2 -> vout1 (3.3V) */
			val->val1 = (rail == 2) ? (3300 + drift * 3)
						: (12000 + drift * 10);
			break;
		case SENSOR_CHAN_POWER:
			/* pin1 ~12.0W (12000000 uW), pout1 ~3.0W (3000000 uW) */
			val->val1 = (rail == 1) ? (12000000 + drift * 20000)
						: (3000000 + drift * 5000);
			break;
		case SENSOR_CHAN_CURRENT:
			/* iout1 ~1.0A (1000 mA) */
			val->val1 = 1000 + drift * 2;
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			/* temp1 ~35.0C (35000 mC) */
			val->val1 = 35000 + drift * 100;
			break;
		case SENSOR_CHAN_RPM:
			/* fan1 ~5000 RPM */
			val->val1 = 5000 + drift * 20;
			break;
		default:
			val->val1 = 0;
			break;
	}
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api fake_psu_api = {
	.sample_fetch = fake_psu_sample_fetch,
	.channel_get = fake_psu_channel_get,
};

static int fake_psu_init(const struct device *dev)
{
	struct fake_psu_data *data = dev->data;

	data->tick = 0;
	return 0;
}

#define FAKE_PSU_DEFINE(inst)                                              \
	static struct fake_psu_data fake_psu_data_##inst;                 \
	DEVICE_DT_INST_DEFINE(inst, fake_psu_init, NULL,                  \
			      &fake_psu_data_##inst, NULL, POST_KERNEL,   \
			      CONFIG_SENSOR_INIT_PRIORITY,                 \
			      &fake_psu_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_PSU_DEFINE)
