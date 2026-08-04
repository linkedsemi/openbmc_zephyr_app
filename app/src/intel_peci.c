/* Real Intel PECI CPU temperature / power / DIMM sensor driver.
 *
 * This is the "real hardware" counterpart of fake_peci_cputemp.c. Same Zephyr
 * sensor API contract -> same hwmon_i2c meas layout -> same IntelCPUSensor
 * dbus object. When an Intel CPU is wired to the board you swap
 *
 *   test,fake-peci-cputemp   (self-test stub)
 *   for
 *   intel,peci-cputemp       (this driver)
 *
 * by changing the hwmon-ins phandle in DT. No IntelCPUSensorMain / hwmon_i2c
 * changes are needed.
 *
 * Enable from prj.conf (NOT a -D...=y build flag -- those are reserved for
 * fake self-test sensors):
 *
 *   CONFIG_PECI=y
 *   CONFIG_PECI_LS=y
 *   CONFIG_SENSOR_INTEL_PECI=y
 *
 * The matching DT overlay is applied automatically by app/CMakeLists.txt.
 *
 * ============================================================
 * Protocol layer (PECI 3.0)
 * ============================================================
 *   Ping                  -> verify CPU presence
 *   RdPkgCfg(host, 16)     -> read Tjmax at init
 *   GetTemp0              -> die temperature (degC)
 *   RdPkgCfg(idx=14)      -> DIMM A1 / DIMM B1 (CPU IMC reports them
 *                            through the memory thermal domain; NOT
 *                            via a separate I2C/SMBus tsensor on the
 *                            DIMM module -- the CPU's integrated
 *                            memory controller is the source of truth)
 *   RdPkgCfg(idx=30)      -> TDP_UNITS (PU/EU/TU, one-shot at init)
 *   RdPkgCfg(idx=3,4)     -> Package / DRAM energy accumulator (RAPL),
 *                            two-shot delta -> watts in sample_fetch
 *
 * PECI addresses: 0x30..0x37 for CPU 0..7.
 *
 * ============================================================
 * Zephyr sensor API contract
 * ============================================================
 *   channel_get()  uses val->val2 as the rail selector (same numbering as
 *                  fake_peci_cputemp.c -- this is THE interface contract
 *                  that hwmon_i2c + the IntelCPUSensor dbus object depend on):
 *
 *      temp   val2=0 -> DTS          (degC, from PECI GetTemp0)
 *      temp   val2=1 -> Margin       (degC, = Tjmax - DTS)
 *      temp   val2=2 -> DIMM A1      (degC, from PECI memory thermal domain)
 *      temp   val2=3 -> DIMM B1      (degC, from PECI memory thermal domain)
 *      power  val2=0 -> Package      (W)
 *      power  val2=1 -> DRAM         (W)
 *
 *   Units are the physical ones the IntelCPUSensor JSON config expects
 *   (Scale 0 for temperatures, Scale -6 for power).
 *
 * ============================================================
 * Caching
 * ============================================================
 *   - Tjmax is read once at init (or taken from DT `tjmax`) and cached.
 *   - DTS, Margin, DIMM A1, DIMM B1, Package, DRAM are refreshed in
 *     sample_fetch(). hwmon_i2c calls sample_fetch() at most once per
 *     HWMON_CACHE_MS (5 s) per hwmon node, so a single PECI burst per
 *     fetch is the upper bound.
 *
 * ============================================================
 * DIMM temperatures (all via PECI, no I2C/SMBus tsensor on the DIMM)
 * ============================================================
 *   DIMM A1 / DIMM B1 come from the CPU's integrated memory controller
 *   (IMC) thermal sensors, exposed indirectly through PECI. Two PECI
 *   mechanisms are commonly used depending on the CPU family:
 *
 *     a) RdPkgCfg with a memory-domain index (newer Xeon Scalable):
 *        The CPU's Package Configuration space includes memory thermal
 *        status under a specific index. The exact index is family-
 *        specific (e.g. 0x0E on Skylake-SP/Cascade Lake, 0x10 on Ice
 *        Lake). See Intel PECI 3.x "Memory Thermal Management" chapter.
 *
 *     b) RdIAMSR to read IA32_PACKAGE_THERM_STATUS / DRAM_THERM_STATUS
 *        MSRs (wider CPU coverage, slightly different bit layout):
 *        MSR 0x1A4 / 0x1A5 / 0x1B1 carry memory thermal data. The
 *        driver picks whichever the platform supports.
 *
 *   Both reach the BMC ONLY through PECI -- no DIMM-side tsensor /
 *   I2C/SMBus path. The DIMM slot SMBus is reserved for SPD / PMBus
 *   and is not the source of the temperature reading.
 */

#define DT_DRV_COMPAT intel_peci_cputemp

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/peci.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(intel_peci, CONFIG_SENSOR_LOG_LEVEL);

/* ---- Rail indices (must match fake_peci_cputemp.c exactly) ---- */
#define RAIL_TEMP_DTS    0
#define RAIL_TEMP_MARGIN 1
#define RAIL_TEMP_DIMMA  2
#define RAIL_TEMP_DIMMB  3
#define RAIL_PWR_PACKAGE 0
#define RAIL_PWR_DRAM    1

/* ---- PECI 3.0 client (host) addresses ---- */
#define PECI_HOST_ADDR_CPU0 0x30u
#define PECI_HOST_ADDR_CPU1 0x31u
/* PECI 3.0 reserves 0x30..0x37; address 0x00 is broadcast. */

/* RdPkgConfig constants: 4-byte request, 5-byte (DWORD) response.
 * index 16 holds the Tjmax (in degC) under the host ID 0. */
#define PECI_RD_PKG_HOST_ID    0u
#define PECI_RD_PKG_INDEX_TJ   16u
#define PECI_RD_PKG_PARAM_LSB  0u
#define PECI_RD_PKG_PARAM_MSB  0u

/* Power / Energy PECI mailbox indices (RAPL via PECI 3.0).
 *
 * The CPU exposes the same RAPL (Running Average Power Limit) energy
 * accumulators that the in-kernel intel-rapl driver reads from MSR
 * space -- but here the BMC accesses them through PECI RdPkgConfig so
 * the host does not have to be running an OS. Per the openbmc
 * peci-ioctl.h header (which mirrors Intel's own PECI mailbox
 * definition):
 *
 *   TDP_UNITS   = 30 -- power / energy / time unit bitfields, read once
 *   ENERGY_COUNTER = 3  -- Package (PCU) energy accumulator, 32-bit
 *   ENERGY_STATUS  = 4  -- DRAM (memory controller) energy accumulator
 *
 * The energy accumulators tick at the unit rates set by TDP_UNITS:
 *   1 energy unit  = 1 / 2^EU joules
 *   1 time unit    = 1 / 2^TU seconds (CPU internal counter; we use
 *                    wall-clock time on the BMC side and ignore TU in
 *                    the formula below)
 *
 * Two-shot algorithm (per hwmon_i2c HWMON_CACHE_MS cadence, ~5 s):
 *
 *      E0 at t0, E1 at t1
 *      delta_E = (int32_t)(E1 - E0)            // signed -> wrap-around
 *      P_watts = delta_E / 2^EU / (t1 - t0)_s
 */
#define PECI_MBX_TDP_UNITS    30u
#define PECI_MBX_PKG_ENERGY    3u
#define PECI_MBX_DRAM_ENERGY   4u

/* Memory-domain package config indices. These are the indices that
 * expose the CPU's integrated memory controller (IMC) thermal status
 * through PECI RdPkgCfg. The CPU monitors the DIMM thermistors (TSODs)
 * itself and reports the result here -- no DIMM-side I2C/SMBus tsensor
 * is involved.
 *
 *   PECI_DIMM_TEMP_IDX  -- Skylake-SP / Cascade Lake (Xeon Scalable,
 *                           server CPUs). The IMC reports per-channel
 *                           DIMM temperatures in a packed DWORD; the
 *                           driver picks channel 0 for DIMM A1 and
 *                           channel 1 for DIMM B1.
 *
 *   PECI_DRAM_THERM_MSR -- Ice Lake / Sapphire Rapids and newer client
 *                           Xeons, where the memory thermal MSR
 *                           (0x1B1 / IA32_PACKAGE_THERM_STATUS family
 *                           extended) carries the per-DIMM reading.
 *
 * The driver picks whichever the platform supports; the TODO in
 * intel_peci_read_dimm_temps() marks the selection point. */
#define PECI_DIMM_TEMP_IDX        0x0Eu /* Xeon Scalable memory thermal */
#define PECI_DRAM_THERM_MSR       0x1B1u /* extended package therm status */

struct intel_peci_config {
	const struct device *peci_dev; /* &peci1 -- bound by DT_INST */
	uint8_t cpu_addr;              /* PECI client address, e.g. 0x30 */
	int8_t tjmax_dc;               /* degC, or -1 to read at init */
};

struct intel_peci_data {
	/* Cached sensor values (refreshed in sample_fetch). */
	int dts_dc;          /* die temp, degC */
	int margin_dc;       /* Tjmax - DTS, degC */
	int package_w;       /* package power, W (0 if not implemented) */
	int dram_w;          /* DRAM power, W (0 if not implemented) */
	int dimm_a_dc;       /* DIMM A1, from PECI memory-thermal domain */
	int dimm_b_dc;       /* DIMM B1, from PECI memory-thermal domain */

	/* Cached Tjmax. Valid after init if sample_fetch has been called at
	 * least once. */
	int tjmax_dc;

	/* RAPL units from TDP_UNITS mailbox (read once at init). The actual
	 * multipliers are 1 / 2^EU joules and 1 / 2^PU watts; the driver
	 * only needs EU for the power-from-energy-delta formula. */
	uint8_t pu;          /* power unit, 0 if TDP_UNITS not yet read */
	uint8_t eu;          /* energy unit, 0 if TDP_UNITS not yet read */
	uint8_t tu;          /* time unit, unused but cached for completeness */

	/* Last energy readings + uptime in ms, for two-shot power
	 * calculation. valid==false until the second sample_fetch call. */
	bool pkg_valid;
	uint32_t pkg_energy_prev;
	int64_t pkg_time_prev_ms;
	bool dram_valid;
	uint32_t dram_energy_prev;
	int64_t dram_time_prev_ms;
};

/* ---- PECI helpers ---- */

/* Read Tjmax via RdPkgConfig(host=0, index=16). PECI RdPkgConfig request:
 *   [HostID(1)] [Index(1)] [ParamLSB(1)] [ParamMSB(1)]
 * response is a DWORD; byte 0 is the PECI completion code, bytes 1..4 are
 * the value. Returns 0 on success and writes degC to *out; negative errno
 * on transport / completion-code failure. */
static int intel_peci_read_tjmax(const struct device *peci_dev,
				 uint8_t addr, int *out)
{
	uint8_t tx[PECI_RD_PKG_WR_LEN] = {
		PECI_RD_PKG_HOST_ID,
		PECI_RD_PKG_INDEX_TJ,
		PECI_RD_PKG_PARAM_LSB,
		PECI_RD_PKG_PARAM_MSB,
	};
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1] = {0};
	struct peci_msg msg = {
		.addr = addr,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = { .buf = tx, .len = sizeof(tx) },
		.rx_buffer = { .buf = rx, .len = PECI_RD_PKG_LEN_DWORD },
	};
	int ret = peci_transfer(peci_dev, &msg);

	if (ret < 0) {
		return ret;
	}
	if (rx[0] != PECI_CC_RSP_SUCCESS) {
		LOG_ERR("RdPkgConfig(Tjmax) CC=0x%02x", rx[0]);
		return -EIO;
	}
	/* Tjmax lives in the LSB of the response DWORD. */
	*out = (int)rx[1];
	return 0;
}

/* GetTemp0 returns the actual die temperature. The raw two bytes are
 * sign-extended 16-bit (the upper bits are the negative margin to 0), and
 * added to Tjmax to give degC -- the same algorithm the upstream Zephyr
 * PECI sample uses (see samples/drivers/peci/src/main.c). */
static int intel_peci_read_dts(const struct device *peci_dev, uint8_t addr,
			       int tjmax_dc, int *out_dc)
{
	uint8_t rx[PECI_GET_TEMP_RD_LEN + 1] = {0};
	struct peci_msg msg = {
		.addr = addr,
		.cmd_code = PECI_CMD_GET_TEMP0,
		.tx_buffer = { .buf = NULL, .len = PECI_GET_TEMP_WR_LEN },
		.rx_buffer = { .buf = rx, .len = PECI_GET_TEMP_RD_LEN },
	};
	int ret = peci_transfer(peci_dev, &msg);

	if (ret < 0) {
		return ret;
	}
	if (rx[0] != PECI_CC_RSP_SUCCESS) {
		LOG_ERR("GetTemp0 CC=0x%02x", rx[0]);
		return -EIO;
	}

	int16_t raw = (int16_t)((uint16_t)rx[0] |
				((uint16_t)rx[1] << 8));

	if (raw == (int16_t)0x8000) {
		/* General sensor error -- the upstream sample substitutes a
		 * "safe" 72 C; do the same so the dbus value stays finite. */
		*out_dc = 72;
		return -EAGAIN;
	}

	/* Strip the 6 LSBs (sub-degree resolution not used here) and
	 * subtract from Tjmax to get degC. */
	raw = (raw >> 6) | 0x7E00;
	*out_dc = (int)raw + tjmax_dc;
	return 0;
}

/* Read DIMM A1 / DIMM B1 via the CPU's memory thermal domain. The CPU's
 * integrated memory controller (IMC) monitors the DIMM thermistors and
 * reports the result through PECI; the BMC does NOT poll any DIMM-side
 * I2C/SMBus tsensor for this reading.
 *
 * Two PECI mechanisms are supported and the platform picks one:
 *
 *   (a) RdPkgCfg with PECI_DIMM_TEMP_IDX (memory thermal domain) --
 *       Xeon Scalable (Skylake-SP / Cascade Lake). Response is a DWORD
 *       with per-channel DIMM temperatures packed: bytes 1..2 = channel
 *       0 (DIMM A1), bytes 3..4 = channel 1 (DIMM B1), each in 0.1 C
 *       unsigned units relative to a 0 C base.
 *
 *   (b) RdIAMSR to read PECI_DRAM_THERM_MSR (extended package therm
 *       status) -- Ice Lake / Sapphire Rapids and newer client Xeons.
 *       The MSR layout is platform-specific but the high byte of the
 *       returned QWORD typically encodes per-DIMM temperatures.
 *
 * The default in this driver is mechanism (a) because that is the most
 * widely supported server path. The TODO below marks where to switch to
 * (b) for client CPUs / non-server parts. Returns 0 on success and
 * writes degC integers to *out_a and *out_b. */
static int intel_peci_read_dimm_temps(const struct device *peci_dev,
				      uint8_t addr,
				      int *out_a_dc, int *out_b_dc)
{
	uint8_t tx[PECI_RD_PKG_WR_LEN] = {
		/* HostID=0 (CPU's own thermal domain), index = memory
		 * thermal config, param = 0 selects the "current" snapshot
		 * of the per-channel DIMM temperatures. */
		0u, (uint8_t)PECI_DIMM_TEMP_IDX, 0u, 0u,
	};
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1] = {0};
	struct peci_msg msg = {
		.addr = addr,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = { .buf = tx, .len = sizeof(tx) },
		.rx_buffer = { .buf = rx, .len = PECI_RD_PKG_LEN_DWORD },
	};
	int ret = peci_transfer(peci_dev, &msg);

	if (ret < 0) {
		return ret;
	}
	if (rx[0] != PECI_CC_RSP_SUCCESS) {
		LOG_WRN("RdPkgCfg(DIMM temp) CC=0x%02x", rx[0]);
		return -EIO;
	}

	/* TODO(platform): confirm the byte packing for the actual CPU
	 * family on the board. The Skylake-SP / Cascade Lake reference
	 * (Intel doc 558284 §6.2) puts the two per-channel DIMM
	 * temperatures in bytes 1..2 and 3..4 of the response DWORD, in
	 * 0.1 C units (uint16, little-endian). The exact layout can differ
	 * on other CPU families -- the platform port replaces this block
	 * with the family-specific decode, or switches to RdIAMSR with
	 * PECI_DRAM_THERM_MSR for the client Xeons. */
	uint16_t ch0_raw = (uint16_t)rx[1] | ((uint16_t)rx[2] << 8);
	uint16_t ch1_raw = (uint16_t)rx[3] | ((uint16_t)rx[4] << 8);

	*out_a_dc = (int)(ch0_raw / 10u);   /* 0.1 C -> degC */
	*out_b_dc = (int)(ch1_raw / 10u);
	return 0;
}

/* Read TDP_UNITS (mailbox index 30) once at init. The DWORD response
 * bit layout per the Intel PECI 3.0 spec:
 *
 *   bits  3:0  power_unit   (PU) -- 1 / 2^PU watts
 *   bits 12:8  energy_unit  (EU) -- 1 / 2^EU joules
 *   bits 19:16 time_unit    (TU) -- 1 / 2^TU seconds
 *
 * Only EU is needed for the energy-delta -> power calculation, but PU
 * and TU are cached in case a future caller wants the same scaling. */
static int intel_peci_read_power_units(const struct device *peci_dev,
				      uint8_t addr,
				      uint8_t *out_pu, uint8_t *out_eu,
				      uint8_t *out_tu)
{
	uint8_t tx[PECI_RD_PKG_WR_LEN] = {
		0u, (uint8_t)PECI_MBX_TDP_UNITS, 0u, 0u,
	};
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1] = {0};
	struct peci_msg msg = {
		.addr = addr,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = { .buf = tx, .len = sizeof(tx) },
		.rx_buffer = { .buf = rx, .len = PECI_RD_PKG_LEN_DWORD },
	};
	int ret = peci_transfer(peci_dev, &msg);

	if (ret < 0) {
		return ret;
	}
	if (rx[0] != PECI_CC_RSP_SUCCESS) {
		LOG_WRN("RdPkgCfg(TDP_UNITS) CC=0x%02x", rx[0]);
		return -EIO;
	}

	uint32_t raw = (uint32_t)rx[1] | ((uint32_t)rx[2] << 8) |
		       ((uint32_t)rx[3] << 16) | ((uint32_t)rx[4] << 24);

	*out_pu = (uint8_t)((raw >> 0)  & 0x0Fu);
	*out_eu = (uint8_t)((raw >> 8)  & 0x1Fu);
	*out_tu = (uint8_t)((raw >> 16) & 0x0Fu);
	return 0;
}

/* Read a 32-bit RAPL energy accumulator (Package = index 3, DRAM = 4).
 * The response DWORD is in little-endian byte order in rx[1..4]. */
static int intel_peci_read_energy(const struct device *peci_dev, uint8_t addr,
				  uint8_t mbx_index, uint32_t *out_energy)
{
	uint8_t tx[PECI_RD_PKG_WR_LEN] = { 0u, mbx_index, 0u, 0u };
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1] = {0};
	struct peci_msg msg = {
		.addr = addr,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = { .buf = tx, .len = sizeof(tx) },
		.rx_buffer = { .buf = rx, .len = PECI_RD_PKG_LEN_DWORD },
	};
	int ret = peci_transfer(peci_dev, &msg);

	if (ret < 0) {
		return ret;
	}
	if (rx[0] != PECI_CC_RSP_SUCCESS) {
		LOG_WRN("RdPkgCfg(energy idx=%u) CC=0x%02x", mbx_index, rx[0]);
		return -EIO;
	}

	*out_energy = (uint32_t)rx[1] |
		      ((uint32_t)rx[2] << 8) |
		      ((uint32_t)rx[3] << 16) |
		      ((uint32_t)rx[4] << 24);
	return 0;
}

/* Convert an (energy_prev, energy_now, time_dt_ms) triple into a power
 * reading in watts, applying the EU scaling from TDP_UNITS. Returns
 * true on success, false if time_dt_ms is too small to be meaningful. */
static bool intel_peci_energy_delta_to_watts(uint32_t prev, uint32_t now,
					     int64_t dt_ms, uint8_t eu,
					     int *out_watts)
{
	if (dt_ms <= 0) {
		return false;
	}

	/* Unsigned subtraction in C is mod 2^32, so a 32-bit counter
	 * wrap-around (now < prev) gives the right positive delta
	 * automatically. */
	uint32_t delta = now - prev;

	/* Scale to joules: 1 energy unit = 1 / 2^EU joules. Use a 64-bit
	 * intermediate to avoid 32-bit overflow when EU is small
	 * (high-energy systems, or long sample intervals). */
	int64_t energy_mj = ((int64_t)delta * 1000) >> eu;   /* millijoules */

	/* Convert to watts: P = E / t. dt is in ms -> divide by 1000. */
	int64_t p_mw = (energy_mj * 1000) / dt_ms;          /* milliwatts */

	*out_watts = (int)(p_mw / 1000);
	return true;
}

/* ---- Zephyr sensor API ---- */

static int intel_peci_sample_fetch(const struct device *dev,
				   enum sensor_channel chan)
{
	struct intel_peci_data *data = dev->data;
	const struct intel_peci_config *cfg = dev->config;
	int ret;

	ARG_UNUSED(chan);

	/* Lazy Tjmax read when not provided by DT. */
	if (data->tjmax_dc <= 0 && cfg->tjmax_dc < 0) {
		ret = intel_peci_read_tjmax(cfg->peci_dev, cfg->cpu_addr,
					    &data->tjmax_dc);
		if (ret < 0) {
			LOG_ERR("Tjmax read failed: %d", ret);
			return ret;
		}
		LOG_INF("Tjmax = %d C (cpu 0x%02x)", data->tjmax_dc,
			cfg->cpu_addr);
	} else if (data->tjmax_dc <= 0) {
		data->tjmax_dc = cfg->tjmax_dc;
	}

	/* DTS via GetTemp0. */
	int dts = 0;

	ret = intel_peci_read_dts(cfg->peci_dev, cfg->cpu_addr,
				  data->tjmax_dc, &dts);
	if (ret == 0 || ret == -EAGAIN) {
		data->dts_dc = dts;
		data->margin_dc = data->tjmax_dc - dts;
	} else {
		LOG_WRN("GetTemp0 failed (%d); keeping previous values", ret);
		/* Leave previous values in place so a transient PECI glitch
		 * does not zero out the dbus property. */
	}

	/* Package + DRAM power via RAPL energy accumulators exposed through
	 * the PECI package-configuration mailbox. TDP_UNITS is read lazily
	 * the first time we get here (cached afterwards). The energy
	 * accumulators themselves need two shots: store on the first call,
	 * compute on the second and later. */
	if (data->eu == 0) {
		uint8_t pu = 0, eu = 0, tu = 0;

		ret = intel_peci_read_power_units(cfg->peci_dev,
						  cfg->cpu_addr,
						  &pu, &eu, &tu);
		if (ret == 0) {
			data->pu = pu;
			data->eu = eu;
			data->tu = tu;
			LOG_INF("TDP_UNITS: PU=%u EU=%u TU=%u (1 EU = %u uJ)",
				pu, eu, tu, (unsigned)(1000000u >> eu));
		} else {
			LOG_WRN("TDP_UNITS read failed (%d); power disabled",
				ret);
			data->pu = data->eu = data->tu = 0;
		}
	}

	if (data->eu > 0) {
		int64_t now_ms = (int64_t)k_uptime_get();
		uint32_t pkg_e = 0, dram_e = 0;
		bool pkg_ok = (intel_peci_read_energy(cfg->peci_dev,
						      cfg->cpu_addr,
						      PECI_MBX_PKG_ENERGY,
						      &pkg_e) == 0);
		bool dram_ok = (intel_peci_read_energy(cfg->peci_dev,
						       cfg->cpu_addr,
						       PECI_MBX_DRAM_ENERGY,
						       &dram_e) == 0);

		if (pkg_ok && data->pkg_valid) {
			int w = 0;

			if (intel_peci_energy_delta_to_watts(
				    data->pkg_energy_prev, pkg_e,
				    now_ms - data->pkg_time_prev_ms,
				    data->eu, &w)) {
				data->package_w = w;
			}
		}
		if (pkg_ok) {
			data->pkg_energy_prev = pkg_e;
			data->pkg_time_prev_ms = now_ms;
			data->pkg_valid = true;
		}

		if (dram_ok && data->dram_valid) {
			int w = 0;

			if (intel_peci_energy_delta_to_watts(
				    data->dram_energy_prev, dram_e,
				    now_ms - data->dram_time_prev_ms,
				    data->eu, &w)) {
				data->dram_w = w;
			}
		}
		if (dram_ok) {
			data->dram_energy_prev = dram_e;
			data->dram_time_prev_ms = now_ms;
			data->dram_valid = true;
		}

		if (!pkg_ok || !dram_ok) {
			LOG_WRN("Energy read failed (pkg_ok=%d dram_ok=%d); "
				"keeping previous power values",
				(int)pkg_ok, (int)dram_ok);
		}
	}

	/* DIMM A1 / DIMM B1 come from the CPU's integrated memory
	 * controller through the PECI memory-thermal domain. Not from a
	 * DIMM-side I2C/SMBus tsensor -- see file-top comment. */
	int dimm_a = 0, dimm_b = 0;

	ret = intel_peci_read_dimm_temps(cfg->peci_dev, cfg->cpu_addr,
					 &dimm_a, &dimm_b);
	if (ret == 0) {
		data->dimm_a_dc = dimm_a;
		data->dimm_b_dc = dimm_b;
	} else {
		LOG_WRN("DIMM temp read failed (%d); keeping previous values",
			ret);
	}

	return 0;
}

static int intel_peci_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct intel_peci_data *data = dev->data;
	int rail = (int)val->val2;

	ARG_UNUSED(dev);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		switch (rail) {
		case RAIL_TEMP_DTS:
			val->val1 = data->dts_dc;
			break;
		case RAIL_TEMP_MARGIN:
			val->val1 = data->margin_dc;
			break;
		case RAIL_TEMP_DIMMA:
			val->val1 = data->dimm_a_dc;
			break;
		case RAIL_TEMP_DIMMB:
			val->val1 = data->dimm_b_dc;
			break;
		default:
			val->val1 = 0;
			break;
		}
		break;

	case SENSOR_CHAN_POWER:
		switch (rail) {
		case RAIL_PWR_PACKAGE:
			val->val1 = data->package_w;
			break;
		case RAIL_PWR_DRAM:
			val->val1 = data->dram_w;
			break;
		default:
			val->val1 = 0;
			break;
		}
		break;

	default:
		val->val1 = 0;
		break;
	}
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api intel_peci_api = {
	.sample_fetch = intel_peci_sample_fetch,
	.channel_get = intel_peci_channel_get,
};

static int intel_peci_init(const struct device *dev)
{
	struct intel_peci_data *data = dev->data;
	const struct intel_peci_config *cfg = dev->config;

	if (!device_is_ready(cfg->peci_dev)) {
		LOG_ERR("PECI controller %s not ready", cfg->peci_dev->name);
		return -ENODEV;
	}

	/* Cache DT-supplied Tjmax so first sample_fetch can use it without
	 * an extra RdPkgConfig. */
	if (cfg->tjmax_dc > 0) {
		data->tjmax_dc = cfg->tjmax_dc;
	}

	data->dts_dc = 0;
	data->margin_dc = 0;
	data->package_w = 0;
	data->dram_w = 0;
	data->dimm_a_dc = 0;
	data->dimm_b_dc = 0;

	LOG_INF("intel_peci ready: peci=%s cpu_addr=0x%02x tjmax=%s",
		cfg->peci_dev->name, cfg->cpu_addr,
		(cfg->tjmax_dc > 0) ? "from-dt" : "from-peci");
	return 0;
}

#define INTEL_PECI_INIT(inst)                                                    \
	static struct intel_peci_data intel_peci_data_##inst;                  \
                                                                               \
	static const struct intel_peci_config intel_peci_cfg_##inst = {        \
		.peci_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, peci_controller)), \
		.cpu_addr = (uint8_t)DT_INST_PROP(inst, cpu_address),          \
		.tjmax_dc = (int8_t)DT_INST_PROP_OR(inst, tjmax, -1),           \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(inst,                                             \
			      intel_peci_init,                                  \
			      NULL,                                             \
			      &intel_peci_data_##inst,                          \
			      &intel_peci_cfg_##inst,                           \
			      POST_KERNEL,                                      \
			      CONFIG_SENSOR_INIT_PRIORITY,                      \
			      &intel_peci_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_PECI_INIT)
