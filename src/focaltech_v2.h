#ifndef _FOCALTECH_V2_H
#define _FOCALTECH_V2_H

#define FOC_MAX_FINGERS 4

struct focaltech_finger_state
{
    int x;
    int y;
    int Width;
    int Length;
    bool active;
};


struct focaltech_hw_state {
	struct focaltech_finger_state fingers[FOC_MAX_FINGERS];
    int fingerCount;
	bool leftPresseed;
	bool rightPresseed;
};

struct focaltech_data {
	bool isReadNext;
	int infoData[16];
	unsigned int x_max, y_max;
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