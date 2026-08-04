/* Fake PECI CPU temperature/power sensor driver -- self-test only
 * (CONFIG_FAKE_PECI_CPUTEMP).
 *
 * Returns slowly-varying temperature and power values through the Zephyr
 * sensor API so the hwmon_i2c -> sysfs *_input -> IntelCPUSensor dbus path
 * can be exercised without a real Intel CPU connected.
 *
 * Instantiate from a DT overlay:
 *
 *   / {
 *       fake_peci_cpu0: fake-peci-cputemp {
 *           compatible = "test,fake-peci-cputemp";
 *           #hwmon-in-cells = <1>;
 *           status = "okay";
 *       };
 *       hwmon_peci_cpu0: hwmon_peci_cpu0 {
 *           compatible = "linkedsemi,hwmon";
 *           class = <0x5>;
 *           label = "peci_cputemp";
 *           sensor-type = "temperature";
 *           hwmon-ins = <&fake_peci_cpu0 0>;
 *           meas = "temp1_input:DTS", "temp2_input:Margin",
 *                  "power1_input:cpu power", "power2_input:dimm power",
 *                  "temp3_input:DIMM A1", "temp4_input:DIMM B1";
 *           status = "okay";
 *       };
 *   };
 *
 * hwmon_i2c requests SENSOR_CHAN_AMBIENT_TEMP / POWER and passes
 * (file index - 1) as val2 -- the rail / sub-channel selector.
 * The IntelCPUSensor code sets scale=0 for temperatures and scale=-6 for
 * power, so this driver returns physical values directly:
 *   temp*_input  -> degrees C   (no scaling on dbus)
 *   power*_input -> Watts       (scale -6: raw*10^6 -> uW -> W)
 *
 * Wait: IntelCPUSensor scale for "cpu power" is -6, meaning
 *   dbus Value = rawValue / 10^-6 = rawValue * 10^6
 * If we want dbus to show ~65W, rawValue should be 65 * 10^-6 ?
 * No. Let's look at how Sensor::updateValue works:
 *   rawValue = newValue * 10^scale
 *   dbus Value = rawValue / 10^scale = newValue
 * So dbus Value equals the string read from sysfs.
 * For power with scale=-6:
 *   rawValue = newValue * 10^-6
 *   dbus Value = rawValue / 10^-6 = newValue * 10^6
 * Wait, that's wrong. Let's re-read...
 *
 * Actually in Sensor::updateValue(double newValue):
 *   rawValue = newValue * std::pow(10, scale);
 * And when reading dbus property "Value":
 *   value = rawValue / std::pow(10, scale);
 *
 * So if scale = -6 and sysfs reads "65":
 *   newValue = 65.0
 *   rawValue = 65.0 * 10^-6 = 0.000065
 *   dbus Value = 0.000065 / 10^-6 = 65.0
 * This is correct! The sysfs value IS the physical value (Watts).
 *
 * For temperature with scale = 0 and sysfs reads "45":
 *   newValue = 45.0
 *   rawValue = 45.0 * 10^0 = 45.0
 *   dbus Value = 45.0 / 10^0 = 45.0
 * Also correct.
 *
 * So this driver returns physical values directly (degC, W).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT test_fake_peci_cputemp

struct fake_peci_data {
	int tick; /* 0..99, drifts ~1/s */
};

static int fake_peci_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	struct fake_peci_data *data = dev->data;

	ARG_UNUSED(chan);
	data->tick = (int)(k_uptime_get_32() / 1000U) % 100;
	return 0;
}

static int fake_peci_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct fake_peci_data *data = dev->data;
	/* hwmon_i2c pre-sets val2 = (file index - 1) as the rail selector:
	 *   temp1_input  -> 0 (DTS),      temp2_input  -> 1 (Margin)
	 *   power1_input -> 0 (cpu power), power2_input -> 1 (dimm power)
	 *   temp3_input  -> 2 (DIMM A1),   temp4_input  -> 3 (DIMM B1) */
	int rail = (int)val->val2;
	/* Center drift at 0 so readings vary slowly around the nominal value. */
	int drift = data->tick - 50;

	ARG_UNUSED(dev);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Return degrees C directly (IntelCPUSensor scale = 0):
		 *   rail 0 (DTS)       -> ~45 C (CPU package)
		 *   rail 1 (Margin)    -> ~15 C (margin to Tjmax)
		 *   rail 2 (DIMM A1)   -> ~35 C
		 *   rail 3 (DIMM B1)   -> ~35 C */
		if (rail == 0) {
			val->val1 = 45 + drift / 5;
		} else if (rail == 1) {
			val->val1 = 15 + drift / 10;
		} else {
			val->val1 = 35 + drift / 8;
		}
		break;
	case SENSOR_CHAN_POWER:
		/* Return Watts directly (IntelCPUSensor scale = -6):
		 *   rail 0 (cpu power)  -> ~65 W
		 *   rail 1 (dimm power) -> ~12 W */
		if (rail == 0) {
			val->val1 = 65 + drift / 3;
		} else {
			val->val1 = 12 + drift / 10;
		}
		break;
	default:
		val->val1 = 0;
		break;
	}
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api fake_peci_api = {
	.sample_fetch = fake_peci_sample_fetch,
	.channel_get = fake_peci_channel_get,
};

static int fake_peci_init(const struct device *dev)
{
	struct fake_peci_data *data = dev->data;

	data->tick = 0;
	return 0;
}

#define FAKE_PECI_DEFINE(inst)                                          \
	static struct fake_peci_data fake_peci_data_##inst;                \
	DEVICE_DT_INST_DEFINE(inst, fake_peci_init, NULL,                  \
			      &fake_peci_data_##inst, NULL, POST_KERNEL, \
			      CONFIG_SENSOR_INIT_PRIORITY,           \
			      &fake_peci_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_PECI_DEFINE)
