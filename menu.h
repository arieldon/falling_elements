#ifndef MENU_H
#define MENU_H

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

	s32 CommandCount;
	quad Commands[64];
};

static menu_context MenuContext;

static void MenuBegin(void);
static void MenuEnd(void);

static void MenuInputMouseMove(s32 X, s32 Y);
static void MenuInputMouseButtonPress(s32 X, s32 Y);
static void MenuInputMouseButtonRelease(s32 X, s32 Y);

#define MenuButton(IconColor, Label) MenuButtonID(__LINE__, IconColor, Label)

#endif
