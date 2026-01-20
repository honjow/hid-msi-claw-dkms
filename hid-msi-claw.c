#include <linux/dmi.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/mutex.h>

//#include "hid-ids.h"

#define MSI_CLAW_FEATURE_GAMEPAD_REPORT_ID 0x0f

#define MSI_CLAW_READ_SIZE 64
#define MSI_CLAW_WRITE_SIZE 64

#define MSI_CLAW_GAME_CONTROL_DESC   0x05
#define MSI_CLAW_DEVICE_CONTROL_DESC 0x06

enum msi_claw_gamepad_mode {
	MSI_CLAW_GAMEPAD_MODE_OFFLINE = 0x00,
	MSI_CLAW_GAMEPAD_MODE_XINPUT = 0x01,
	MSI_CLAW_GAMEPAD_MODE_DINPUT = 0x02,
	MSI_CLAW_GAMEPAD_MODE_MSI = 0x03,
	MSI_CLAW_GAMEPAD_MODE_DESKTOP = 0x04,
	MSI_CLAW_GAMEPAD_MODE_BIOS = 0x05,
	MSI_CLAW_GAMEPAD_MODE_TESTING = 0x06,

	MSI_CLAW_GAMEPAD_MODE_MAX,
};

enum msi_claw_mkeys_function {
	MSI_CLAW_MKEY_FUNCTION_MACRO = 0x00,
	MSI_CLAW_MKEY_FUNCTION_COMBINATION = 0x01,
	MSI_CLAW_MKEY_FUNCTION_DISABLED = 0x02,

	MSI_CLAW_MKEY_FUNCTION_MAX,
};

static const bool gamepad_mode_debug = false;

static const struct {
	const char* name;
	const bool available;
} gamepad_mode_map[] = {
	{"offline", gamepad_mode_debug},
	{"xinput", true},
	{"dinput", gamepad_mode_debug},
	{"msi", gamepad_mode_debug},
	{"desktop", true},
	{"bios", gamepad_mode_debug},
	{"testing", gamepad_mode_debug},
};

static const char* mkeys_function_map[] =
{
	"macro",
	"combination",
};

#define MSI_CLAW_M_REMAP_MAX_KEYS 5

enum msi_claw_m_key {
	MSI_CLAW_M1_KEY = 0,
	MSI_CLAW_M2_KEY = 1,

	MSI_CLAW_M_KEY_MAX,
};

static const struct {
	const char *name;
	uint8_t code;
} m_remap_key_map[] = {
	/* Gamepad buttons */
	{"BTN_DPAD_UP", 0x01},
	{"BTN_DPAD_DOWN", 0x02},
	{"BTN_DPAD_LEFT", 0x03},
	{"BTN_DPAD_RIGHT", 0x04},
	{"BTN_TL", 0x05},
	{"BTN_TR", 0x06},
	{"BTN_THUMBL", 0x07},
	{"BTN_THUMBR", 0x08},
	{"BTN_SOUTH", 0x09},
	{"BTN_EAST", 0x0a},
	{"BTN_NORTH", 0x0b},
	{"BTN_WEST", 0x0c},
	{"BTN_MODE", 0x0d},
	{"BTN_SELECT", 0x0e},
	{"BTN_START", 0x0f},
	/* Keyboard keys */
	{"KEY_ESC", 0x32},
	{"KEY_F1", 0x33},
	{"KEY_F2", 0x34},
	{"KEY_F3", 0x35},
	{"KEY_F4", 0x36},
	{"KEY_F5", 0x37},
	{"KEY_F6", 0x38},
	{"KEY_F7", 0x39},
	{"KEY_F8", 0x3a},
	{"KEY_F9", 0x3b},
	{"KEY_F10", 0x3c},
	{"KEY_F11", 0x3d},
	{"KEY_F12", 0x3e},
	{"KEY_GRAVE", 0x3f},
	{"KEY_1", 0x40},
	{"KEY_2", 0x41},
	{"KEY_3", 0x42},
	{"KEY_4", 0x43},
	{"KEY_5", 0x44},
	{"KEY_6", 0x45},
	{"KEY_7", 0x46},
	{"KEY_8", 0x47},
	{"KEY_9", 0x48},
	{"KEY_0", 0x49},
	{"KEY_MINUS", 0x4a},
	{"KEY_EQUAL", 0x4b},
	{"KEY_BACKSPACE", 0x4c},
	{"KEY_TAB", 0x4d},
	{"KEY_Q", 0x4e},
	{"KEY_W", 0x4f},
	{"KEY_E", 0x50},
	{"KEY_R", 0x51},
	{"KEY_T", 0x52},
	{"KEY_Y", 0x53},
	{"KEY_U", 0x54},
	{"KEY_I", 0x55},
	{"KEY_O", 0x56},
	{"KEY_P", 0x57},
	{"KEY_LEFTBRACE", 0x58},
	{"KEY_RIGHTBRACE", 0x59},
	{"KEY_BACKSLASH", 0x5a},
	{"KEY_CAPSLOCK", 0x5b},
	{"KEY_A", 0x5c},
	{"KEY_S", 0x5d},
	{"KEY_D", 0x5e},
	{"KEY_F", 0x5f},
	{"KEY_G", 0x60},
	{"KEY_H", 0x61},
	{"KEY_J", 0x62},
	{"KEY_K", 0x63},
	{"KEY_L", 0x64},
	{"KEY_SEMICOLON", 0x65},
	{"KEY_LEFTSHIFT", 0x66},
	{"KEY_APOSTROPHE", 0x67},
	{"KEY_ENTER", 0x68},
	{"KEY_Z", 0x69},
	{"KEY_X", 0x6a},
	{"KEY_C", 0x6b},
	{"KEY_V", 0x6c},
	{"KEY_B", 0x6d},
	{"KEY_N", 0x6e},
	{"KEY_M", 0x6f},
	{"KEY_LEFTCTRL", 0x70},
	{"KEY_RIGHTSHIFT", 0x71},
	{"KEY_COMMA", 0x72},
	{"KEY_DOT", 0x73},
	{"KEY_SLASH", 0x74},
	{"KEY_LEFTALT", 0x75},
	{"KEY_LEFTMETA", 0x76},
	{"KEY_RIGHTCTRL", 0x77},
	{"KEY_RIGHTALT", 0x78},
	{"KEY_SPACE", 0x79},
	{"KEY_INSERT", 0x7a},
	{"KEY_HOME", 0x7b},
	{"KEY_PAGEUP", 0x7c},
	{"KEY_DELETE", 0x7d},
	{"KEY_END", 0x7e},
	{"KEY_PAGEDOWN", 0x7f},
	{"KEY_KPENTER", 0x8a},
	{"KEY_KP0", 0x8b},
	{"KEY_KP1", 0x8c},
	{"KEY_KP2", 0x8d},
	{"KEY_KP3", 0x8e},
	{"KEY_KP4", 0x8f},
	{"KEY_KP5", 0x90},
	{"KEY_KP6", 0x91},
	{"KEY_KP7", 0x92},
	{"KEY_KP8", 0x93},
	{"KEY_KP9", 0x94},
	/* Disabled */
	{"disabled", 0xff},
};

static const uint8_t m_remap_addr_old[MSI_CLAW_M_KEY_MAX][2] = {
	{0x00, 0x7a},  /* M1 */
	{0x01, 0x1f},  /* M2 */
};

static const uint8_t m_remap_addr_new[MSI_CLAW_M_KEY_MAX][2] = {
	{0x00, 0xbb},  /* M1 */
	{0x01, 0x64},  /* M2 */
};

enum msi_claw_command_type {
	MSI_CLAW_COMMAND_TYPE_ENTER_PROFILE_CONFIG = 0x01,
	MSI_CLAW_COMMAND_TYPE_EXIT_PROFILE_CONFIG = 0x02,
	MSI_CLAW_COMMAND_TYPE_WRITE_PROFILE = 0x03,
	MSI_CLAW_COMMAND_TYPE_READ_PROFILE = 0x04,
	MSI_CLAW_COMMAND_TYPE_READ_PROFILE_ACK = 0x05,
	// ACK is read after a WRITE_RGB_STATUS
	MSI_CLAW_COMMAND_TYPE_ACK = 0x06,
	MSI_CLAW_COMMAND_TYPE_SWITCH_PROFILE = 0x07,
	MSI_CLAW_COMMAND_TYPE_WRITE_PROFILE_TO_EEPROM = 0x08,
	MSI_CLAW_COMMAND_TYPE_SYNC_RGB = 0x09,
	MSI_CLAW_COMMAND_TYPE_READ_RGB_STATUS_ACK = 0x0a,
	MSI_CLAW_COMMAND_TYPE_READ_CURRENT_PROFILE = 0x0b,
	MSI_CLAW_COMMAND_TYPE_READ_CURRENT_PROFILE_ACK = 0x0c,
	MSI_CLAW_COMMAND_TYPE_READ_RGB_STATUS = 0x0d,
	// TODO: 0f00003c210100b137ff00000000ff00000000ff00000000ff00000000ff00000000ff00000000ff00000000ff00000000ff00000000ff00000000ff00000000
	MSI_CLAW_COMMAND_TYPE_WRITE_RGB_STATUS = 0x21,
	MSI_CLAW_COMMAND_TYPE_SYNC_TO_ROM = 0x22,
	MSI_CLAW_COMMAND_TYPE_RESTORE_FROM_ROM = 0x23,
	MSI_CLAW_COMMAND_TYPE_SWITCH_MODE = 0x24,
	MSI_CLAW_COMMAND_TYPE_READ_GAMEPAD_MODE = 0x26,
	MSI_CLAW_COMMAND_TYPE_GAMEPAD_MODE_ACK = 0x27,
	MSI_CLAW_COMMAND_TYPE_RESET_DEVICE = 0x28,
	MSI_CLAW_COMMAND_TYPE_RGB_CONTROL = 0xe0,
	MSI_CLAW_COMMAND_TYPE_CALIBRATION_CONTROL = 0xfd,
	MSI_CLAW_COMMAND_TYPE_CALIBRATION_ACK = 0xfe,
};

struct msi_claw_control_status {
	enum msi_claw_gamepad_mode gamepad_mode;
	enum msi_claw_mkeys_function mkeys_function;
};

struct msi_claw_read_data {
	const uint8_t *data;
	int size;

	struct msi_claw_read_data *next;
};

struct msi_claw_drvdata {
	struct hid_device *hdev;

	//struct input_dev *input;

	struct msi_claw_control_status *control;

	struct mutex read_data_mutex;
	struct msi_claw_read_data *read_data;

	/* M key remap support */
	u16 bcd_device;
	bool m_remap_supported;
	const uint8_t (*m_remap_addr)[2];
};

static void msi_claw_flush_queue(struct hid_device *hdev);

static int msi_claw_write_cmd(struct hid_device *hdev, enum msi_claw_command_type cmdtype,
    const uint8_t *const buffer, size_t buffer_len)
{
	int ret;
	uint8_t *dmabuf = NULL;
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	const uint8_t buf[MSI_CLAW_WRITE_SIZE] = {
		MSI_CLAW_FEATURE_GAMEPAD_REPORT_ID, 0, 0, 0x3c, cmdtype };

	msi_claw_flush_queue(hdev);

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		ret = -ENODEV;
		goto msi_claw_write_cmd_err;
	}

	if (buffer != NULL)
		memcpy((void *)&buf[5], buffer, buffer_len);
	else
		buffer_len = 0;

	memset((void *)&buf[5 + buffer_len], 0, MSI_CLAW_WRITE_SIZE - (5 + buffer_len));
	dmabuf = kmemdup(buf, MSI_CLAW_WRITE_SIZE, GFP_KERNEL);
	if (!dmabuf) {
		ret = -ENOMEM;
		hid_err(hdev, "hid-msi-claw failed to alloc dma buf: %d\n", ret);
		goto msi_claw_write_cmd_err;
	}

	ret = hid_hw_output_report(hdev, dmabuf, MSI_CLAW_WRITE_SIZE);
	if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw failed to switch controller mode: %d\n", ret);
		goto msi_claw_write_cmd_err;
	}

	hid_notice(hdev, "hid-msi-claw sent %d bytes, cmd: 0x%02x\n", ret, dmabuf[4]);

msi_claw_write_cmd_err:
	kfree(dmabuf);

	return ret;
}

static int msi_claw_read(struct hid_device *hdev, uint8_t *const buffer, int size, uint32_t timeout)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	struct msi_claw_read_data *event = NULL;
	int ret = 0;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		ret = -ENODEV;
		goto msi_claw_read_err;
	}

	for (uint32_t i = 0; (event == NULL) && (i <= timeout); i++) {
		if (timeout - i)
			msleep(20);

		scoped_guard(mutex, &drvdata->read_data_mutex) {
			event = drvdata->read_data;

			if (event != NULL)
				drvdata->read_data = event->next;
		};
	}

	if (event == NULL) {
		ret = -EIO;
		hid_err(hdev, "hid-msi-claw no answer from device\n");
		goto msi_claw_read_err;
	}

	if (size < event->size) {
		ret = -EINVAL;
		hid_err(hdev, "hid-msi-claw invalid buffer size: too short\n");
		goto msi_claw_read_err;
	}

	memcpy((void *)buffer, (const void *)event->data, event->size);
	ret = event->size;

msi_claw_read_err:
	if (event != NULL) {
		kfree(event->data);
		kfree(event);
	}

	return ret;
}

static void msi_claw_flush_queue(struct hid_device *hdev)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	struct msi_claw_read_data *event, *next;

	scoped_guard(mutex, &drvdata->read_data_mutex) {
		event = drvdata->read_data;
		drvdata->read_data = NULL;

		while (event) {
			next = event->next;
			kfree(event->data);
			kfree(event);
			event = next;
		}
	}
}

static int msi_claw_raw_event_control(struct hid_device *hdev, struct msi_claw_drvdata *drvdata,
	struct hid_report *report, uint8_t *data, int size)
{
	struct msi_claw_read_data **list = NULL;
	struct msi_claw_read_data *node = NULL;
	uint8_t *buffer;
	int ret, i;

	if (size != MSI_CLAW_READ_SIZE) {
		//hid_err(hdev, "hid-msi-claw got unknown %d bytes\n", size);
		goto msi_claw_raw_event_control_err;
	} else if (data[0] != 0x10) {
		hid_err(hdev, "hid-msi-claw unrecognised byte at offset 0: expected 0x10, got 0x%02x\n", data[0]);
		goto msi_claw_raw_event_control_err;
	} else if (data[1] != 0x00) {
		hid_err(hdev, "hid-msi-claw unrecognised byte at offset 1: expected 0x00, got 0x%02x\n", data[1]);
		goto msi_claw_raw_event_control_err;
	} else if (data[2] != 0x00) {
		hid_err(hdev, "hid-msi-claw unrecognised byte at offset 2: expected 0x00, got 0x%02x\n", data[2]);
		goto msi_claw_raw_event_control_err;
	} else if (data[3] != 0x3c) {
		hid_err(hdev, "hid-msi-claw unrecognised byte at offset 3: expected 0x3c, got 0x%02x\n", data[3]);
		goto msi_claw_raw_event_control_err;
	}

	buffer = kmemdup(data, size, GFP_KERNEL);
	if (buffer == NULL) {
		ret = -ENOMEM;
		hid_err(hdev, "hid-msi-claw failed to alloc %d bytes for read buffer: %d\n", size, ret);
		goto msi_claw_raw_event_control_err;
	}

	struct msi_claw_read_data evt = {
		.data = buffer,
		.size = size,
		.next = NULL,
	};

	node = kmemdup(&evt, sizeof(evt), GFP_KERNEL);
	if (!node) {
		ret = -ENOMEM;
		kfree(buffer);
		hid_err(hdev, "hid-msi-claw failed to alloc event node: %d\n", ret);
		goto msi_claw_raw_event_control_err;
	}

	scoped_guard(mutex, &drvdata->read_data_mutex) {
		list = &drvdata->read_data;
		for (i = 0; (i < 32) && (*list != NULL); i++)
			list = &(*list)->next;

		if (*list != NULL) {
			ret = -EIO;
			hid_err(hdev, "too many unparsed events: ignoring\n");
			goto msi_claw_raw_event_control_err;
		}

		*list = node;
	}

	hid_notice(hdev, "hid-msi-claw received %d bytes, cmd: 0x%02x\n", size, buffer[4]);

	return 0;

msi_claw_raw_event_control_err:
	if (buffer != NULL)
		kfree(buffer);

	if (node != NULL)
		kfree(node);

	return ret;
}

static int msi_claw_raw_event(struct hid_device *hdev, struct hid_report *report, uint8_t *data, int size)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	if (!drvdata->control) {
		hid_notice(hdev, "hid-msi-claw event not from control interface: ignoring\n");
		return 0;
	}

	return msi_claw_raw_event_control(hdev, drvdata, report, data, size);
}

static int msi_claw_await_ack(struct hid_device *hdev)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	uint8_t buffer[MSI_CLAW_READ_SIZE];
	int ret;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		return -ENODEV;
	}

	ret = msi_claw_read(hdev, buffer, MSI_CLAW_READ_SIZE, 1000);
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to read ack: %d\n", ret);
		return ret;
	} else if (ret != MSI_CLAW_READ_SIZE) {
		hid_err(hdev, "hid-msi-claw invalid read: expected %d bytes, got %d\n", MSI_CLAW_READ_SIZE, ret);
		return -EINVAL;
	}

	if (buffer[4] != (uint8_t)MSI_CLAW_COMMAND_TYPE_ACK) {
		hid_err(hdev, "hid-msi-claw expected ACK (0x06), got 0x%02x\n", buffer[4]);
		return -EINVAL;
	}

	return 0;
}

static int sync_to_rom(struct hid_device *hdev)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	int ret;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		ret = -ENODEV;
		goto sync_to_rom_err;
	}

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_SYNC_TO_ROM, NULL, 0);
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to send write request for switch controller mode: %d\n", ret);
		goto sync_to_rom_err;
	} else if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw failed to send the sync to rom command: %d\n", ret);
		ret = -EIO;
		goto sync_to_rom_err;
	}

	ret = msi_claw_await_ack(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to await first ack: %d\n", ret);
		goto sync_to_rom_err;
	}

	// the sync to rom also triggers two ack
	ret = msi_claw_await_ack(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to await second ack: %d\n", ret);
		goto sync_to_rom_err;
	}

	ret = 0;

sync_to_rom_err:
	return ret;
}

static bool msi_claw_fw_supports_m_remap(u16 bcd_device,
	const uint8_t (**addr)[2])
{
	u8 major = bcd_device >> 8;

	if (major == 1) {
		*addr = (bcd_device >= 0x0166) ?
			m_remap_addr_new : m_remap_addr_old;
		return true;
	}

	if (major == 2 && bcd_device >= 0x0217) {
		*addr = m_remap_addr_new;
		return true;
	}

	if (major >= 3) {
		*addr = m_remap_addr_new;
		return true;
	}

	return false;
}

static int m_remap_name_to_code(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(m_remap_key_map); i++) {
		if (!strcmp(name, m_remap_key_map[i].name))
			return m_remap_key_map[i].code;
	}

	return -EINVAL;
}

static const char *m_remap_code_to_name(uint8_t code)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(m_remap_key_map); i++) {
		if (m_remap_key_map[i].code == code)
			return m_remap_key_map[i].name;
	}

	return NULL;
}

static int msi_claw_set_m_remap(struct hid_device *hdev,
	enum msi_claw_m_key m_key, const uint8_t *codes)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	const uint8_t cmd_buffer[] = {
		0x01,
		drvdata->m_remap_addr[m_key][0],
		drvdata->m_remap_addr[m_key][1],
		0x07, 0x04, 0x00,
		codes[0], codes[1], codes[2], codes[3], codes[4],
	};
	int ret;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		ret = -ENODEV;
		goto msi_claw_set_m_remap_err;
	}

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_WRITE_RGB_STATUS,
		cmd_buffer, sizeof(cmd_buffer));
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to set m_remap: %d\n", ret);
		goto msi_claw_set_m_remap_err;
	} else if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw failed to write m_remap: %d\n", ret);
		ret = -EIO;
		goto msi_claw_set_m_remap_err;
	}

	ret = msi_claw_await_ack(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to await ack for m_remap: %d\n", ret);
		goto msi_claw_set_m_remap_err;
	}

	ret = sync_to_rom(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to sync m_remap to rom: %d\n", ret);
		goto msi_claw_set_m_remap_err;
	}

	return 0;

msi_claw_set_m_remap_err:
	return ret;
}

static int msi_claw_read_m_remap(struct hid_device *hdev,
	enum msi_claw_m_key m_key, uint8_t *codes, int *count)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	const uint8_t cmd_buffer[] = {
		0x01,
		drvdata->m_remap_addr[m_key][0],
		drvdata->m_remap_addr[m_key][1],
		0x07,
	};
	uint8_t buffer[MSI_CLAW_READ_SIZE] = {};
	int ret, i;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		return -ENODEV;
	}

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_READ_PROFILE,
		cmd_buffer, sizeof(cmd_buffer));
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to send read m_remap request: %d\n", ret);
		return ret;
	} else if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw couldn't send read m_remap request: %d\n", ret);
		return -EIO;
	}

	ret = msi_claw_read(hdev, buffer, MSI_CLAW_READ_SIZE, 50);
	if (ret != MSI_CLAW_READ_SIZE) {
		hid_err(hdev, "hid-msi-claw failed to read m_remap: %d\n", ret);
		return -EINVAL;
	}

	if (buffer[4] != (uint8_t)MSI_CLAW_COMMAND_TYPE_READ_PROFILE_ACK) {
		hid_err(hdev, "hid-msi-claw expected READ_PROFILE_ACK (0x05), got 0x%02x\n", buffer[4]);
		return -EINVAL;
	}

	/* Extract key codes, filtering out 0xff (disabled/empty) */
	*count = 0;
	for (i = 0; i < MSI_CLAW_M_REMAP_MAX_KEYS; i++) {
		if (buffer[11 + i] != 0xff)
			codes[(*count)++] = buffer[11 + i];
	}

	return 0;
}

static int msi_claw_reset_device(struct hid_device *hdev)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	int ret;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		ret = -ENODEV;
		goto msi_claw_reset_device_err;
	}

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_RESET_DEVICE, NULL, 0);
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to send reset: %d\n", ret);
		goto msi_claw_reset_device_err;
	} else if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw couldn't send reset request: %d\n", ret);
		ret = -EIO;
		goto msi_claw_reset_device_err;
	}

	ret = msi_claw_await_ack(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to await ack: %d\n", ret);
		goto msi_claw_reset_device_err;
	}

msi_claw_reset_device_err:
	return ret;
}

static int msi_claw_read_gamepad_mode(struct hid_device *hdev,
	struct msi_claw_control_status *status)
{
	uint8_t buffer[MSI_CLAW_READ_SIZE] = {};
	int ret;

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_READ_GAMEPAD_MODE, NULL, 0);
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to send read request for controller mode: %d\n", ret);
		goto msi_claw_read_gamepad_mode_err;
	} else if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw couldn't send request: %d\n", ret);
		ret = -EIO;
		goto msi_claw_read_gamepad_mode_err;
	}

	ret = msi_claw_read(hdev, buffer, MSI_CLAW_READ_SIZE, 50);
	if (ret != MSI_CLAW_READ_SIZE) {
		hid_err(hdev, "hid-msi-claw failed to read: %d\n", ret);
		ret = -EINVAL;
		goto msi_claw_read_gamepad_mode_err;
	}

	if (buffer[4] != (uint8_t)MSI_CLAW_COMMAND_TYPE_GAMEPAD_MODE_ACK) {
		hid_err(hdev, "hid-msi-claw received invalid response: expected 0x27, got 0x%02x\n", buffer[4]);
		ret = -EINVAL;
		goto msi_claw_read_gamepad_mode_err;
	} else if (buffer[5] >= MSI_CLAW_GAMEPAD_MODE_MAX) {
		hid_err(hdev, "hid-msi-claw unknown gamepad mode: 0x%02x\n", buffer[5]);
		ret = -EINVAL;
		goto msi_claw_read_gamepad_mode_err;
	} else if (buffer[6] >= MSI_CLAW_MKEY_FUNCTION_MAX) {
		hid_err(hdev, "hid-msi-claw unknown gamepad mode: 0x%02x\n", buffer[6]);
		ret = -EINVAL;
		goto msi_claw_read_gamepad_mode_err;
	}

	status->gamepad_mode = (enum msi_claw_gamepad_mode)buffer[5];
	status->mkeys_function = (enum msi_claw_mkeys_function)buffer[6];

	ret = 0;

msi_claw_read_gamepad_mode_err:
	return ret;
}

static int msi_claw_switch_gamepad_mode(struct hid_device *hdev,
	const struct msi_claw_control_status *status)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	struct msi_claw_control_status check_status;
	const uint8_t cmd_buffer[2] = {(uint8_t)status->gamepad_mode, (uint8_t)status->mkeys_function};
	int ret;

	if (!drvdata->control) {
		hid_err(hdev, "hid-msi-claw couldn't find control interface\n");
		ret = -ENODEV;
		goto msi_claw_switch_gamepad_mode_err;
	}

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_SWITCH_MODE, cmd_buffer, sizeof(cmd_buffer));
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw failed to send write request to switch controller mode: %d\n", ret);
		goto msi_claw_switch_gamepad_mode_err;
	} else if (ret != MSI_CLAW_WRITE_SIZE) {
		hid_err(hdev, "hid-msi-claw failed to write: %d bytes got written\n", ret);
		ret = -EIO;
		goto msi_claw_switch_gamepad_mode_err;
	}

	ret = msi_claw_await_ack(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to await first ack: %d\n", ret);
		goto msi_claw_switch_gamepad_mode_err;
	}

	// the gamepad mode switch mode triggers two ack
	ret = msi_claw_await_ack(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to await second ack: %d\n", ret);
		goto msi_claw_switch_gamepad_mode_err;
	}

	// check the new mode as official application does
	ret = msi_claw_read_gamepad_mode(hdev, &check_status);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to read status: %d\n", ret);
		goto msi_claw_switch_gamepad_mode_err;
	}

	if (memcmp((const void *)&check_status, (const void *)status, sizeof(*status))) {
		hid_err(hdev, "hid-msi-claw current status and target one are different\n");
		ret = -EIO;
		goto msi_claw_switch_gamepad_mode_err;
	}

	// the device now sends back 03 00 00 00 00 00 00 00 00

	// this command is always issued by the windows counterpart after a mode switch
	ret = sync_to_rom(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed the sync to rom command: %d\n", ret);
		return ret;
	}

msi_claw_switch_gamepad_mode_err:
	return ret;
}

static ssize_t reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct hid_device *hdev = to_hid_device(dev);
	int ret;

	ret = msi_claw_reset_device(hdev);
	if (ret < 0) {
		hid_err(hdev, "hid-msi-claw error resetting device: %d\n", ret);
		goto reset_store_err;
	}

	return count;

reset_store_err:
	return ret;
}
static DEVICE_ATTR_WO(reset);

static ssize_t gamepad_mode_available_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, ret = 0;
	int len = ARRAY_SIZE(gamepad_mode_map);

	for (i = 0; i < len; i++) {
		if (!gamepad_mode_map[i].available)
			continue;

		ret += sysfs_emit_at(buf, ret, "%s", gamepad_mode_map[i].name);

		if (i < len-1)
			ret += sysfs_emit_at(buf, ret, " ");
	}
	ret += sysfs_emit_at(buf, ret, "\n");

	return ret;
}
static DEVICE_ATTR_RO(gamepad_mode_available);

static ssize_t gamepad_mode_current_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hid_device *hdev = to_hid_device(dev);
	struct msi_claw_control_status status;
	int ret;

	ret = msi_claw_read_gamepad_mode(hdev, &status);
	if (ret) {
		hid_err(hdev, "hid-msi-claw error reaging the gamepad mode: %d\n", ret);
		return ret;
	}

	return sysfs_emit(buf, "%s\n", gamepad_mode_map[(int)status.gamepad_mode].name);
}

static ssize_t gamepad_mode_current_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	uint8_t *input;
	struct hid_device *hdev = to_hid_device(dev);
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	enum msi_claw_gamepad_mode new_gamepad_mode = ARRAY_SIZE(gamepad_mode_map);
	struct msi_claw_control_status status = {
		.gamepad_mode = drvdata->control->gamepad_mode,
		.mkeys_function = drvdata->control->mkeys_function,
	};

	if (!count) {
		ret = -EINVAL;
		goto gamepad_mode_current_store_err;
	}

	input = kmemdup(buf, count+1, GFP_KERNEL);
	if (!input) {
		ret = -ENOMEM;
		goto gamepad_mode_current_store_err;
	}

	input[count] = '\0';
	if (input[count-1] == '\n')
		input[count-1] = '\0';

	for (size_t i = 0; i < (size_t)new_gamepad_mode; i++)
		if ((!strcmp(input, gamepad_mode_map[i].name)) && (gamepad_mode_map[i].available))
			new_gamepad_mode = (enum msi_claw_gamepad_mode)i;

	kfree(input);

	if (new_gamepad_mode == ARRAY_SIZE(gamepad_mode_map)) {
		hid_err(hdev, "Invalid gamepad mode selected\n");
		ret = -EINVAL;
		goto gamepad_mode_current_store_err;
	}

	status.gamepad_mode = new_gamepad_mode;
	ret = msi_claw_switch_gamepad_mode(hdev, &status);
	if (ret) {
		hid_err(hdev, "Error changing gamepad mode: %d\n", (int)ret);
		goto gamepad_mode_current_store_err;
	}

	ret = count;

gamepad_mode_current_store_err:
	return ret;
}
static DEVICE_ATTR_RW(gamepad_mode_current);

static ssize_t mkeys_function_available_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, ret = 0;
	int len = ARRAY_SIZE(mkeys_function_map);

	for (i = 0; i < len; i++) {
		ret += sysfs_emit_at(buf, ret, "%s", mkeys_function_map[i]);

		if (i < len-1)
			ret += sysfs_emit_at(buf, ret, " ");
	}
	ret += sysfs_emit_at(buf, ret, "\n");

	return ret;
}
static DEVICE_ATTR_RO(mkeys_function_available);

static ssize_t mkeys_function_current_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hid_device *hdev = to_hid_device(dev);
	struct msi_claw_control_status status;
	int ret = msi_claw_read_gamepad_mode(hdev, &status);

	if (ret) {
		hid_err(hdev, "hid-msi-claw error reaging the gamepad mode: %d\n", ret);
		return ret;
	}

	return sysfs_emit(buf, "%s\n", mkeys_function_map[(int)status.mkeys_function]);
}

static ssize_t mkeys_function_current_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	uint8_t *input;
	ssize_t err;
	struct hid_device *hdev = to_hid_device(dev);
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	enum msi_claw_mkeys_function new_mkeys_function = ARRAY_SIZE(mkeys_function_map);
	struct msi_claw_control_status status = {
		.gamepad_mode = drvdata->control->gamepad_mode,
		.mkeys_function = drvdata->control->mkeys_function,
	};

	if (!count)
		return -EINVAL;

	input = kmemdup(buf, count+1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	input[count] = '\0';
	if (input[count-1] == '\n')
		input[count-1] = '\0';

	for (size_t i = 0; i < (size_t)new_mkeys_function; i++)
		if (!strcmp(input, mkeys_function_map[i]))
			new_mkeys_function = i;

	kfree(input);

	if (new_mkeys_function == ARRAY_SIZE(mkeys_function_map)) {
		hid_err(hdev, "Invalid mkeys function selected\n");
		return -EINVAL;
	}

	status.mkeys_function = new_mkeys_function;
	err = msi_claw_switch_gamepad_mode(hdev, &status);
	if (err) {
		hid_err(hdev, "Error changing mkeys function: %d\n", (int)err);
		return err;
	}

	return count;
}
static DEVICE_ATTR_RW(mkeys_function_current);

static ssize_t m_remap_available_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, ret = 0;
	int len = ARRAY_SIZE(m_remap_key_map);

	for (i = 0; i < len; i++) {
		ret += sysfs_emit_at(buf, ret, "%s", m_remap_key_map[i].name);

		if (i < len - 1)
			ret += sysfs_emit_at(buf, ret, " ");
	}
	ret += sysfs_emit_at(buf, ret, "\n");

	return ret;
}
static DEVICE_ATTR_RO(m_remap_available);

static ssize_t m1_remap_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hid_device *hdev = to_hid_device(dev);
	uint8_t codes[MSI_CLAW_M_REMAP_MAX_KEYS];
	const char *name;
	int key_count, ret, i;
	ssize_t len = 0;

	ret = msi_claw_read_m_remap(hdev, MSI_CLAW_M1_KEY, codes, &key_count);
	if (ret)
		return ret;

	if (key_count == 0)
		return sysfs_emit(buf, "disabled\n");

	for (i = 0; i < key_count; i++) {
		name = m_remap_code_to_name(codes[i]);
		if (name)
			len += sysfs_emit_at(buf, len, "%s", name);
		else
			len += sysfs_emit_at(buf, len, "0x%02x", codes[i]);

		if (i < key_count - 1)
			len += sysfs_emit_at(buf, len, " ");
	}
	len += sysfs_emit_at(buf, len, "\n");

	return len;
}

static ssize_t m1_remap_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hid_device *hdev = to_hid_device(dev);
	uint8_t codes[MSI_CLAW_M_REMAP_MAX_KEYS];
	char *input, *token, *cur;
	int code, ret, key_count = 0, i;

	if (!count)
		return -EINVAL;

	input = kmemdup(buf, count + 1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	input[count] = '\0';
	if (input[count - 1] == '\n')
		input[count - 1] = '\0';

	/* Parse space-separated key names */
	cur = input;
	while ((token = strsep(&cur, " ")) != NULL) {
		if (*token == '\0')
			continue;

		if (key_count >= MSI_CLAW_M_REMAP_MAX_KEYS) {
			hid_err(hdev, "hid-msi-claw too many keys (max %d)\n",
				MSI_CLAW_M_REMAP_MAX_KEYS);
			kfree(input);
			return -EINVAL;
		}

		code = m_remap_name_to_code(token);
		if (code < 0) {
			hid_err(hdev, "hid-msi-claw invalid key: %s\n", token);
			kfree(input);
			return -EINVAL;
		}

		codes[key_count++] = (uint8_t)code;
	}
	kfree(input);

	if (key_count == 0)
		return -EINVAL;

	/* Fill remaining slots with 0xff (disabled) */
	for (i = key_count; i < MSI_CLAW_M_REMAP_MAX_KEYS; i++)
		codes[i] = 0xff;

	ret = msi_claw_set_m_remap(hdev, MSI_CLAW_M1_KEY, codes);
	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(m1_remap);

static ssize_t m2_remap_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hid_device *hdev = to_hid_device(dev);
	uint8_t codes[MSI_CLAW_M_REMAP_MAX_KEYS];
	const char *name;
	int key_count, ret, i;
	ssize_t len = 0;

	ret = msi_claw_read_m_remap(hdev, MSI_CLAW_M2_KEY, codes, &key_count);
	if (ret)
		return ret;

	if (key_count == 0)
		return sysfs_emit(buf, "disabled\n");

	for (i = 0; i < key_count; i++) {
		name = m_remap_code_to_name(codes[i]);
		if (name)
			len += sysfs_emit_at(buf, len, "%s", name);
		else
			len += sysfs_emit_at(buf, len, "0x%02x", codes[i]);

		if (i < key_count - 1)
			len += sysfs_emit_at(buf, len, " ");
	}
	len += sysfs_emit_at(buf, len, "\n");

	return len;
}

static ssize_t m2_remap_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hid_device *hdev = to_hid_device(dev);
	uint8_t codes[MSI_CLAW_M_REMAP_MAX_KEYS];
	char *input, *token, *cur;
	int code, ret, key_count = 0, i;

	if (!count)
		return -EINVAL;

	input = kmemdup(buf, count + 1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	input[count] = '\0';
	if (input[count - 1] == '\n')
		input[count - 1] = '\0';

	/* Parse space-separated key names */
	cur = input;
	while ((token = strsep(&cur, " ")) != NULL) {
		if (*token == '\0')
			continue;

		if (key_count >= MSI_CLAW_M_REMAP_MAX_KEYS) {
			hid_err(hdev, "hid-msi-claw too many keys (max %d)\n",
				MSI_CLAW_M_REMAP_MAX_KEYS);
			kfree(input);
			return -EINVAL;
		}

		code = m_remap_name_to_code(token);
		if (code < 0) {
			hid_err(hdev, "hid-msi-claw invalid key: %s\n", token);
			kfree(input);
			return -EINVAL;
		}

		codes[key_count++] = (uint8_t)code;
	}
	kfree(input);

	if (key_count == 0)
		return -EINVAL;

	/* Fill remaining slots with 0xff (disabled) */
	for (i = key_count; i < MSI_CLAW_M_REMAP_MAX_KEYS; i++)
		codes[i] = 0xff;

	ret = msi_claw_set_m_remap(hdev, MSI_CLAW_M2_KEY, codes);
	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(m2_remap);

static int __maybe_unused msi_claw_resume(struct hid_device *hdev)
{
	int ret;
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);
	struct msi_claw_control_status status = {
		.gamepad_mode = drvdata->control->gamepad_mode,
		.mkeys_function = drvdata->control->mkeys_function,
	};

	// TODO: clear out events list here (or in suspend?)

	// wait for device to be ready
	msleep(500);

	ret = msi_claw_switch_gamepad_mode(hdev, &status);
	if (ret) {
		hid_err(hdev, "Error changing gamepad mode: %d\n", (int)ret);
		goto msi_claw_resume_err;
	}

	// TODO: retry until this works?

	return 0;

msi_claw_resume_err:
	return ret;
}

static int msi_claw_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret;
	struct msi_claw_drvdata *drvdata;

	if (!hid_is_usb(hdev)) {
		hid_err(hdev, "hid-msi-claw hid not usb\n");
		return -ENODEV;
	}

	drvdata = devm_kzalloc(&hdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL) {
		hid_err(hdev, "hid-msi-claw can't alloc descriptor\n");
		return -ENOMEM;
	}

	mutex_init(&drvdata->read_data_mutex);
	drvdata->read_data = NULL;
	drvdata->control = NULL;

	hid_set_drvdata(hdev, drvdata);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw hid parse failed: %d\n", ret);
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hid-msi-claw hw start failed: %d\n", ret);
		return ret;
	}

	ret = hid_hw_open(hdev);
	if (ret) {
		hid_err(hdev, "hid-msi-claw failed to open HID device: %d\n", ret);
		goto err_stop_hw;
	}

	/* Get firmware version for m_remap support */
	{
		struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
		struct usb_device *udev = interface_to_usbdev(intf);
		drvdata->bcd_device = le16_to_cpu(udev->descriptor.bcdDevice);
	}

	drvdata->m_remap_supported = msi_claw_fw_supports_m_remap(
		drvdata->bcd_device, &drvdata->m_remap_addr);

	if (!drvdata->m_remap_supported)
		hid_warn(hdev, "hid-msi-claw firmware 0x%04x not supported for m_remap\n",
			drvdata->bcd_device);

	if (hdev->rdesc[0] == MSI_CLAW_DEVICE_CONTROL_DESC) {
		drvdata->control = devm_kzalloc(&hdev->dev, sizeof(*(drvdata->control)), GFP_KERNEL);
		if (drvdata->control == NULL) {
			hid_err(hdev, "hid-msi-claw can't alloc control interface data\n");
			ret = -ENOMEM;
			goto err_close;
		}

		drvdata->control->gamepad_mode = MSI_CLAW_GAMEPAD_MODE_XINPUT;
		drvdata->control->mkeys_function = MSI_CLAW_MKEY_FUNCTION_MACRO;

		ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_gamepad_mode_available.attr);
		if (ret) {
			hid_err(hdev, "hid-msi-claw failed to sysfs_create_file dev_attr_gamepad_mode_available: %d\n", ret);
			goto err_close;
		}

		ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_gamepad_mode_current.attr);
		if (ret) {
			hid_err(hdev, "hid-msi-claw failed to sysfs_create_file dev_attr_gamepad_mode_current: %d\n", ret);
			goto err_dev_attr_gamepad_mode_current;
		}

		ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_mkeys_function_available.attr);
		if (ret) {
			hid_err(hdev, "hid-msi-claw failed to sysfs_create_file dev_attr_mkeys_function_available: %d\n", ret);
			goto err_dev_attr_mkeys_function_available;
		}

		ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_mkeys_function_current.attr);
		if (ret) {
			hid_err(hdev, "hid-msi-claw failed to sysfs_create_file dev_attr_mkeys_function_current: %d\n", ret);
			goto err_dev_attr_mkeys_function_current;
		}

		ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_reset.attr);
		if (ret) {
			hid_err(hdev, "hid-msi-claw failed to sysfs_create_file dev_attr_reset: %d\n", ret);
			goto err_dev_attr_reset;
		}

		if (drvdata->m_remap_supported) {
			ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_m_remap_available.attr);
			if (ret) {
				hid_err(hdev, "hid-msi-claw failed to create m_remap_available: %d\n", ret);
				goto err_m_remap_available;
			}

			ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_m1_remap.attr);
			if (ret) {
				hid_err(hdev, "hid-msi-claw failed to create m1_remap: %d\n", ret);
				goto err_m1_remap;
			}

			ret = sysfs_create_file(&hdev->dev.kobj, &dev_attr_m2_remap.attr);
			if (ret) {
				hid_err(hdev, "hid-msi-claw failed to create m2_remap: %d\n", ret);
				goto err_m2_remap;
			}
		}
	}

	return 0;

err_m2_remap:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_m1_remap.attr);
err_m1_remap:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_m_remap_available.attr);
err_m_remap_available:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_reset.attr);
err_dev_attr_gamepad_mode_current:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_gamepad_mode_available.attr);
err_dev_attr_mkeys_function_available:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_gamepad_mode_current.attr);
err_dev_attr_mkeys_function_current:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_mkeys_function_available.attr);
err_dev_attr_reset:
	sysfs_remove_file(&hdev->dev.kobj, &dev_attr_mkeys_function_current.attr);
err_close:
	hid_hw_close(hdev);
err_stop_hw:
	hid_hw_stop(hdev);
	return ret;
}

static void msi_claw_remove(struct hid_device *hdev)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	if (drvdata->control) {
		if (drvdata->m_remap_supported) {
			sysfs_remove_file(&hdev->dev.kobj, &dev_attr_m_remap_available.attr);
			sysfs_remove_file(&hdev->dev.kobj, &dev_attr_m1_remap.attr);
			sysfs_remove_file(&hdev->dev.kobj, &dev_attr_m2_remap.attr);
		}
		sysfs_remove_file(&hdev->dev.kobj, &dev_attr_gamepad_mode_available.attr);
		sysfs_remove_file(&hdev->dev.kobj, &dev_attr_gamepad_mode_current.attr);
		sysfs_remove_file(&hdev->dev.kobj, &dev_attr_mkeys_function_available.attr);
		sysfs_remove_file(&hdev->dev.kobj, &dev_attr_mkeys_function_current.attr);
		sysfs_remove_file(&hdev->dev.kobj, &dev_attr_reset.attr);
	}

	hid_hw_close(hdev);
	hid_hw_stop(hdev);
}

static const struct hid_device_id msi_claw_devices[] = {
	{ HID_USB_DEVICE(0x0DB0, 0x1901) },
	{ }
};
MODULE_DEVICE_TABLE(hid, msi_claw_devices);

static struct hid_driver msi_claw_driver = {
	.name			= "hid-msi-claw",
	.id_table		= msi_claw_devices,
	.raw_event		= msi_claw_raw_event,
	.probe			= msi_claw_probe,
	.remove			= msi_claw_remove,
// #ifdef CONFIG_PM
// 	.resume			= msi_claw_resume,
// #endif
};
module_hid_driver(msi_claw_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Denis Benato <benato.denis96@gmail.com>");
MODULE_DESCRIPTION("Manage MSI Claw gamepad device");
