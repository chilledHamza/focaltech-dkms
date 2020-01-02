#ifndef _FOCALTECH_V2_H
#define _FOCALTECH_V2_H

#define FOCALTECH_MAX_FINGERS 4

/* 

MAX_X reported by touchpad is 0x03E0 (992 decimal),
With MAX_X set to 0x03E0 cursor movement is unnatural. 

While MAX_X 0x0550 (1360 decimal) results in quite natural cursor movement,
downside of using 0x0550 is that Horizontal Edge scrolling won't work.

*/

#define MAX_X 	0x08E0  /* 2272 */
#define MAX_Y 	0x0550  /* 1360 */
#define MAX_MAJOR 		10
#define MAX_MINOR 		10
#define MAX_PRESSURE 	127

struct fte_command {
	int command;
	unsigned char data;
};

struct focaltech_finger_state
{
    int x;
    int y;
    int major;
    int minor;
	int pressure;
    bool valid;
};

struct focaltech_hw_state {
	struct focaltech_finger_state fingers[FOCALTECH_MAX_FINGERS];
    int fingerCount;
	bool left;
	bool right;
};

struct focaltech_data {
	bool isReadNext;
	int lastDeviceData[16];
	struct focaltech_hw_state state;
};

#ifdef CONFIG_MOUSE_PS2_FOCALTECH
int focaltech_detect(struct psmouse *psmouse, bool set_properties);
int focaltech_init(struct psmouse *psmouse);
#else
static inline int focaltech_init(struct psmouse *psmouse)
{
	return -ENOSYS;
}
#endif

#endif
