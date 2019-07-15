#include <linux/device.h>
#include <linux/libps2.h>
#include <linux/input/mt.h>
#include <linux/serio.h>
#include <linux/slab.h>
#include "psmouse.h"
#include "focaltech_v2.h"


static const char * const focaltech_pnp_ids[] = {
	"FTE0001",
	NULL
};

int focaltech_detect(struct psmouse *psmouse, bool set_properties)
{
	if (!psmouse_matches_pnp_id(psmouse, focaltech_pnp_ids))
		return -ENODEV;

	if (set_properties) {
		psmouse->vendor = "FocalTech";
		psmouse->name = "Touchpad";
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

	for (i = 0; i < FOC_MAX_FINGERS; i++) {
		struct focaltech_finger_state *finger = &state->fingers[i];
		input_mt_slot(dev, i);
		input_mt_report_slot_state(dev, MT_TOOL_FINGER, finger->active);
		if (finger->active) {
			input_report_abs(dev, ABS_MT_POSITION_X, finger->x);
			input_report_abs(dev, ABS_MT_POSITION_Y, finger->y);
			input_report_abs(dev, ABS_MT_TOUCH_MAJOR, finger->Length);
			input_report_abs(dev, ABS_MT_TOUCH_MINOR, finger->Width);
		}
	}
	input_mt_sync_frame(dev);
	input_report_key(dev, BTN_LEFT, state->leftPresseed);
	input_report_key(dev, BTN_RIGHT, state->rightPresseed);
	input_mt_report_finger_count(dev, state->fingerCount);
	input_sync(dev);
}

static void focaltech_process_packet(struct psmouse *psmouse)
{
    unsigned char *packet = psmouse->packet;
    struct focaltech_data *priv = psmouse->private;
	struct focaltech_hw_state *state = &priv->state;
	int i, j, k, m, n;

    if (!priv->isReadNext)
    {
        for (i = 0; i < 8; i++)
            priv->infoData[i] = packet[i];
        for (i = 8; i < 16; i++)
            priv->infoData[i] = 0xff;
		if ((packet[4] & 3) + ((packet[4] & 48) >> 2) > 2 && (packet[0] != 0xff && packet[1] != 0xff && packet[2] != 0xff && packet[3] != 0xff) && (packet[0] & 48) != 32)
            {
                priv->isReadNext = true;
            }
        state->fingerCount = (int)(packet[4] & 3) + ((packet[4] & 48) >> 2);
    }
    else
    {
        priv->isReadNext = false;
        for (i = 8; i < 16; i++)
            priv->infoData[i] = packet[i-8];
    }
    if (!priv->isReadNext)
    {
        if (!(priv->infoData[0] == 0xff && priv->infoData[1] == 0xff && priv->infoData[2] == 0xff && priv->infoData[3] == 0xff))
        {
            if ((priv->infoData[0] & 1) == 1)   // 0x0d & 0x01 == 1 (Left Button)
                state->leftPresseed = true;
			else
                state->leftPresseed = false;
            if ((priv->infoData[0] & 2) == 2)   // 0x0e & 0x02 == 2 (Right Button)
                state->rightPresseed = true;
			else
			    state->rightPresseed = false;
            if ((priv->infoData[0] & 48) == 16) // 0x1c & 0x30 == 16 (Elipse)
            {
                for (i = 0; i < 4; i++)
                {
                    int j = 0;
                    for (k = (i * (j+4)); j < 4; j++){
                    	if (priv->infoData[k+1] != 0xff && priv->infoData[k+2] != 0xff && priv->infoData[k+3] != 0xff)
                    	{
                        	state->fingers[i].Length = priv->infoData[k+1];
                        	state->fingers[i].Width = priv->infoData[k+2];
                    	}
					}
                }
            }
            else
            {
                for (k = 0; k < 4; k++)
                {
                    m = 0;
                    for (n = k * (m+4); m < 4; m++)
                    {
						if (priv->infoData[n+1] != 0xff && priv->infoData[n+2] != 0xff && priv->infoData[n+3] != 0xff)
                    	{
							state->fingers[k].active = true;
                        	state->fingers[k].x = (priv->infoData[n+1] << 4) + ((priv->infoData[n+3] & 240) >> 4);
                        	state->fingers[k].y = (priv->infoData[n+2] << 4) + (priv->infoData[n+3] & 15);
                    	}
						else
							state->fingers[k].active = false;
                    }
                    
                }
                if (state->fingerCount == 0)
                {
                    for (i = 0; i < 4; i++)
                    {
						state->fingers[i].active = false;
                        state->fingers[i].Length = 0;
                        state->fingers[i].Width = 0;
                    }
                }
            }
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

	/*
	 * We might want to do some validation of the data here, but
	 * we do not know the protocol well enough
	 */
	return PSMOUSE_GOOD_DATA;
}

static int focaltech_switch_protocol(struct psmouse *psmouse)
{
	struct ps2dev *ps2dev = &psmouse->ps2dev;

	if (ps2_command(ps2dev, NULL, 0x00f3))
		return -EIO;

	if (ps2_command(ps2dev, NULL, 0x00ea))
		return -EIO;

	if (ps2_command(ps2dev, NULL, 0x00f3))
		return -EIO;

	if (ps2_command(ps2dev, NULL, 0x00ed))
		return -EIO;

	if (ps2_command(ps2dev, NULL, PSMOUSE_CMD_ENABLE))
		return -EIO;

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
	if (error) {
		psmouse_err(psmouse, "Unable to initialize the device\n");
		return error;
	}

	return 0;
}

static void focaltech_set_input_params(struct psmouse *psmouse)
{
	struct input_dev *dev = psmouse->dev;
	struct focaltech_data *priv = psmouse->private;

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
	input_set_abs_params(dev, ABS_MT_POSITION_X, 0, priv->x_max, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y, 0, priv->y_max, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR, 0, 14, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR, 0, 9, 0, 0);
	input_mt_init_slots(dev, 4, INPUT_MT_POINTER);
}

static int focaltech_read_size(struct psmouse *psmouse)
{
	struct focaltech_data *priv = psmouse->private;

	priv->x_max = 2272; 
	priv->y_max = 992;

	return 0;
}

static void focaltech_set_resolution(struct psmouse *psmouse,
				     unsigned int resolution)
{
	/* not supported yet */
}

static void focaltech_set_rate(struct psmouse *psmouse, unsigned int rate)
{
	/* not supported yet */
}

static void focaltech_set_scale(struct psmouse *psmouse,
				enum psmouse_scale scale)
{
	/* not supported yet */
}

int focaltech_init(struct psmouse *psmouse)
{
	struct focaltech_data *priv;
	int error;

	psmouse->private = priv = kzalloc(sizeof(struct focaltech_data),
					  GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	focaltech_reset(psmouse);

	error = focaltech_read_size(psmouse);
	if (error) {
		psmouse_err(psmouse,
			    "Unable to read the size of the touchpad\n");
		goto fail;
	}

	error = focaltech_switch_protocol(psmouse);
	if (error) {
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