#ifndef PLAT_H
#define PLAT_H

#include "ml.h"

enum shape_tag {
	SH_CIRCLE,
	SH_RECT,
};

struct shape {
	enum shape_tag tag;
	union {
		float radius;
		v2f radii;
	};
};

struct platform {
	int x;
};

struct actor {
	int x;
};

enum body_tag {
	BD_PLATFORM,
	BD_ACTOR,
};

struct body {
	enum body_tag tag;
	v2f pos;
	v2f vel;
	v2f mass;
	v2f invmass;
	union {
		struct platform platform;
		struct actor actor;
	};
};

/*
if (platformRidingPlayers[i])
	playerTryMove(curPlayer, curPlayer->player.x, curPlayer->player.y + fix16_from_int(enemyYInt - enemyOrigYInt), 1);
*/

#endif
