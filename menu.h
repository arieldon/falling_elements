#ifndef MENU_H
#define MENU_H

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

	s32 OffsetX;
	s32 OffsetY;

	union
	{
		struct
		{
			s32 MousePositionX;
			s32 MousePositionY;
		};
		vector2s MousePosition;
	};
	s32 MouseDown;
	s32 PreviousMouseDown;

	u32 HotID;
	u32 ActiveID;

	b32 LoadedIcons;
	u8 Icons[16384];

	s32 CommandCount;
	quad Commands[64];
};

static menu_context MenuContext;

static void MenuBegin(void);
static void MenuEnd(void);

static void MenuInputMouseMove(s32 X, s32 Y);
static void MenuInputMouseButtonPress(s32 X, s32 Y);
static void MenuInputMouseButtonRelease(s32 X, s32 Y);

// TODO(ariel) Ideally, this function shouldn't be exposed. Any functionality
// that needs this call should use it internally.
static inline b32 MouseOverTarget(quad Target);

#define MenuButton(Icon, IconColor, Label) MenuButtonID(__LINE__, Icon, IconColor, Label)

#endif
