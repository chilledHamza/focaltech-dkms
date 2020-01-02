#include <linux/device.h>
#include <linux/libps2.h>
#include <linux/input/mt.h>
#include <linux/serio.h>
#include <linux/slab.h>
#include "psmouse.h"
#include "focaltech_v2.h"

static const struct fte_command switch_protocol[] = {
	{PSMOUSE_CMD_SETRATE, 	 0xea},
	{PSMOUSE_CMD_SETRATE,	 0xed},
	{PSMOUSE_CMD_ENABLE, 	 0x00},
};

static const char *const focaltech_pnp_ids[] = {
	"FTE0001",
	NULL};

int focaltech_detect(struct psmouse *psmouse, bool set_properties)
{
	if (!psmouse_matches_pnp_id(psmouse, focaltech_pnp_ids))
		return -ENODEV;

	if (set_properties)
	{
		psmouse->vendor = "FocalTech";
		psmouse->name = "Touchpad FTE0001";
	}

	return 0;
}

#ifdef CONFIG_MOUSE_PS2_FOCALTECH

static void focaltech_report_state(struct psmouse *psmouse)
{
	struct focaltech_data *priv = psmouse->private;
	struct focaltech_hw_state *state = &priv->state;
	struct input_dev *dev = psmouse->dev;
	int i;

	for (i = 0; i < FOCALTECH_MAX_FINGERS; i++)
	{
		struct focaltech_finger_state *finger = &state->fingers[i];
		input_mt_slot(dev, i);
		input_mt_report_slot_state(dev, MT_TOOL_FINGER, finger->valid);
		if (finger->valid)
		{
			input_report_abs(dev, ABS_MT_POSITION_X, finger->x);
			input_report_abs(dev, ABS_MT_POSITION_Y, finger->y);
			input_report_abs(dev, ABS_MT_TOUCH_MAJOR, finger->major);
			input_report_abs(dev, ABS_MT_TOUCH_MINOR, finger->minor);
			input_report_abs(dev, ABS_MT_PRESSURE, finger->pressure);
		}
	}
	input_mt_sync_frame(dev);
	input_report_key(dev, BTN_LEFT, state->left);
	input_report_key(dev, BTN_RIGHT, state->right);
	input_mt_report_finger_count(dev, state->fingerCount);
	input_sync(dev);
}

static void focaltech_process_packet(struct psmouse *psmouse)
{
	unsigned char *packet = psmouse->packet;
	struct focaltech_data *priv = psmouse->private;
	struct focaltech_hw_state *state = &priv->state;
	int i, j;

	if (!priv->isReadNext)
	{
		for (i = 0; i < 8; i++)
			priv->lastDeviceData[i] = packet[i];
		for (i = 8; i < 16; i++)
			priv->lastDeviceData[i] = 0xff;
		state->fingerCount = (int)(packet[4] & 3) + ((packet[4] & 48) >> 2);
		if ((state->fingerCount > 2) && (packet[0] != 0xff && packet[1] != 0xff && packet[2] != 0xff && packet[3] != 0xff) && (packet[0] & 48) != 32)
		{
			priv->isReadNext = true;
		}
	}
	else
	{
		priv->isReadNext = false;
		for (i = 8; i < 16; i++)
			priv->lastDeviceData[i] = packet[i - 8];
	}
	if (!priv->isReadNext)
	{
		if (!(priv->lastDeviceData[0] == 0xff && priv->lastDeviceData[1] == 0xff && priv->lastDeviceData[2] == 0xff && priv->lastDeviceData[3] == 0xff))
		{
			if ((priv->lastDeviceData[0] & 1) == 1)
				state->left = true;
			else
				state->left = false;
			if ((priv->lastDeviceData[0] & 2) == 2)
				state->right = true;
			else
				state->right = false;
			if ((priv->lastDeviceData[0] & 48) == 16)
			{
				for (i = 0; i < 4; i++)
				{
					j = i * 4;
					if (!((priv->lastDeviceData[j + 1] == 0xff) && (priv->lastDeviceData[j + 2] == 0xff) && (priv->lastDeviceData[j + 3] == 0xff)))
					{
						
						state->fingers[i].minor = priv->lastDeviceData[j + 1];
						state->fingers[i].major = priv->lastDeviceData[j + 2];
						state->fingers[i].pressure = priv->lastDeviceData[j + 3] * 2;
						if (state->fingers[i].pressure > MAX_PRESSURE)
							state->fingers[i].pressure = MAX_PRESSURE;
					}
				}
			}
			else
			{
				for (i = 0; i < 4; i++)
				{
					j = i * 4;
					if (!((priv->lastDeviceData[j + 1] == 0xff) && (priv->lastDeviceData[j + 2] == 0xff) && (priv->lastDeviceData[j + 3] == 0xff)))
					{
						state->fingers[i].valid = true;
						state->fingers[i].x = (priv->lastDeviceData[j + 1] << 4) + ((priv->lastDeviceData[j + 3] & 240) >> 4);
						state->fingers[i].y = (priv->lastDeviceData[j + 2] << 4) + (priv->lastDeviceData[j + 3] & 15);
					}
					else
						state->fingers[i].valid = false;
				}
			}
			if (state->fingerCount == 0)
				for (i = 0; i < 4; i++)
					state->fingers[i].valid = false;
		}
	}
	focaltech_report_state(psmouse);
}

static psmouse_ret_t focaltech_process_byte(struct psmouse *psmouse)
{
	if (psmouse->pktcnt >= 8) { /* Full packet received */
		focaltech_process_packet(psmouse);
		return PSMOUSE_FULL_PACKET;
	}
	return PSMOUSE_GOOD_DATA;
}

static int focaltech_switch_protocol(struct psmouse *psmouse)
{
	struct ps2dev *ps2dev = &psmouse->ps2dev;
	unsigned char param[4];
	size_t i;

	for (i = 0; i < ARRAY_SIZE(switch_protocol); ++i)
	{
		memset(param, 0, sizeof(param));
		param[0] = switch_protocol[i].data;
		if (ps2_command(ps2dev, param, switch_protocol[i].command))
			return -EIO;
	}

	return 0;
}

static void focaltech_reset(struct psmouse *psmouse)
{
	ps2_command(&psmouse->ps2dev, NULL, PSMOUSE_CMD_RESET_DIS);
	psmouse_reset(psmouse);
}

static void focaltech_disconnect(struct psmouse *psmouse)
{
	focaltech_reset(psmouse);
	kfree(psmouse->private);
	psmouse->private = NULL;
}

static int focaltech_reconnect(struct psmouse *psmouse)
{
	int error;

	focaltech_reset(psmouse);

	error = focaltech_switch_protocol(psmouse);
	if (error)
	{
		psmouse_err(psmouse, "Unable to initialize the device\n");
		return error;
	}

	return 0;
}

static void focaltech_set_input_params(struct psmouse *psmouse)
{
	struct input_dev *dev = psmouse->dev;

	/*
	 * Undo part of setup done for us by psmouse core since touchpad
	 * is not a relative device.
	 */
	__clear_bit(EV_REL, dev->evbit);
	__clear_bit(REL_X, dev->relbit);
	__clear_bit(REL_Y, dev->relbit);

	/*
	 * Now set up our capabilities.
	 */
	__set_bit(EV_ABS, dev->evbit);
	input_set_abs_params(dev, ABS_MT_POSITION_X, 0, MAX_X, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y, 0, MAX_Y, 0, 0);
	input_set_abs_params(dev, ABS_MT_PRESSURE, 0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR, 0, MAX_MAJOR, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR, 0, MAX_MINOR, 0, 0);
	input_mt_init_slots(dev, FOCALTECH_MAX_FINGERS, INPUT_MT_POINTER);

	/* 87mm x 40mm */
	input_abs_set_res(dev, ABS_MT_POSITION_X, 26);
	input_abs_set_res(dev, ABS_MT_POSITION_Y, 25);
}

static void focaltech_set_resolution(struct psmouse *psmouse, unsigned int resolution)
{
	/* not supported yet */
}

static void focaltech_set_rate(struct psmouse *psmouse, unsigned int rate)
{
	/* not supported yet */
}

static void focaltech_set_scale(struct psmouse *psmouse, enum psmouse_scale scale)
{
	/* not supported yet */
}

int focaltech_init(struct psmouse *psmouse)
{
	struct focaltech_data *priv;
	int error;

	psmouse->private = priv = kzalloc(sizeof(struct focaltech_data), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	focaltech_reset(psmouse);

	error = focaltech_switch_protocol(psmouse);
	if (error)
	{
		psmouse_err(psmouse, "Unable to initialize the device\n");
		goto fail;
	}

	focaltech_set_input_params(psmouse);

	psmouse->protocol_handler = focaltech_process_byte;
	psmouse->pktsize = 8;
	psmouse->disconnect = focaltech_disconnect;
	psmouse->reconnect = focaltech_reconnect;
	psmouse->cleanup = focaltech_reset;
	/* resync is not supported yet */
	psmouse->resync_time = 0;
	/*
	 * rate/resolution/scale changes are not supported yet, and
	 * the generic implementations of these functions seem to
	 * confuse some touchpads
	 */
	psmouse->set_resolution = focaltech_set_resolution;
	psmouse->set_rate = focaltech_set_rate;
	psmouse->set_scale = focaltech_set_scale;

	return 0;

fail:
	focaltech_reset(psmouse);
	kfree(priv);
	return error;
}
#endif /* CONFIG_MOUSE_PS2_FOCALTECH */
