#ifndef MENU_H
#define MENU_H

#include "icons.inl"

typedef enum menu_icon menu_icon;
enum menu_icon
{
	MENU_ICON_BLANK,
	MENU_ICON_PLAY,
	MENU_ICON_PAUSE,
	MENU_ICON_CLEAR,
	MENU_ICON_COUNT,
} __attribute__((packed));

typedef struct menu_context menu_context;
struct menu_context
{
	union
	{
		struct
		{
			s32 StartOffsetX;
			s32 StartOffsetY;
			s32 Width;
			s32 Height;
		};
		quad EntireMenu;
	};
	b32 MenuIsHot;

	s32 OffsetX;
	s32 OffsetY;

	u32 HotID;
	u32 ActiveID;

	b32 PreviousFrameMouseDown;

	s32 CommandCount;
	quad Commands[64];
};

static menu_context MenuContext;

static void BeginMenu(void);
static void FinishMenu(void);

#define MenuButton(Icon, IconColor) MenuButtonID(__LINE__, Icon, IconColor)

#endif
