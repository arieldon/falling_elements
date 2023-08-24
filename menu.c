enum
{
	ICON_SIZE = 24,
	PADDING = 8,
};

static void
LoadIcons(void)
{
	// TODO(ariel) Include source of image directly in header file.
	u8 Buffer[16445] = {0};

	FILE *IconsFile = fopen("./assets/atlas.pgm", "r");
	Assert(IconsFile);

	u64 ReadSuccess = fread(Buffer, 16445, 1, IconsFile);
	Assert(ReadSuccess == 1);
	Assert(Buffer[0] == 'P' && Buffer[1] == '5');
	fclose(IconsFile);

	memcpy(MenuContext.Icons, &Buffer[61], 16384);
	MenuContext.LoadedIcons = true;
}

static void
BeginMenu(void)
{
	Assert(MenuContext.LoadedIcons);
	b32 HotMenu = MouseOverTarget(MenuContext.EntireMenu);
	MenuContext.OffsetX = MenuContext.StartOffsetX = WINDOW_WIDTH - ICON_SIZE*HotMenu;
	MenuContext.OffsetY = MenuContext.StartOffsetY = WINDOW_HEIGHT/2 - (ICON_SIZE+PADDING)/2 - MenuContext.Height/2;
	MenuContext.Width = 0;
	MenuContext.Height = 0;
	MenuContext.HotID = 0;
	MenuContext.CommandCount = 1;
}

static void
EndMenu(void)
{
	// NOTE(ariel) Don't draw until after first frame because menu isn't centered
	// properly yet.
	if (MenuContext.StartOffsetY == WINDOW_HEIGHT/2 - ICON_SIZE/2)
	{
		MenuContext.CommandCount = 0;
	}

	MenuContext.PreviousMouseDown = MenuContext.MouseDown;

	MenuContext.EntireMenu.X -= ICON_SIZE;
	MenuContext.EntireMenu.Y -= ICON_SIZE / 2;
	MenuContext.EntireMenu.Width += ICON_SIZE;
	MenuContext.EntireMenu.Height += ICON_SIZE;
	MenuContext.EntireMenu.Color = 0xffffffff;
	MenuContext.EntireMenu.TextureID = MENU_ICON_BLANK;
	MenuContext.Commands[0] = MenuContext.EntireMenu;
}

static void
MenuInputMouseMove(s32 X, s32 Y)
{
	MenuContext.MousePositionX = X;
	MenuContext.MousePositionY = Y;
}

static void
MenuInputMouseButtonPress(s32 X, s32 Y)
{
	MenuContext.MouseDown = true;
	MenuInputMouseMove(X, Y);
}

static void
MenuInputMouseButtonRelease(s32 X, s32 Y)
{
	MenuContext.MouseDown = false;
	MenuInputMouseMove(X, Y);
}

static inline b32
MouseOverTarget(quad Target)
{
	b32 X0 = MenuContext.MousePositionX >= Target.X;
	b32 X1 = MenuContext.MousePositionX <= Target.X + Target.Width;
	b32 Y0 = MenuContext.MousePositionY >= Target.Y;
	b32 Y1 = MenuContext.MousePositionY <= Target.Y + Target.Height;
	b32 Result = X0 & X1 & Y0 & Y1;
	return Result;
}

static b32
MenuButtonID(u32 ID, menu_icon Icon, u32 IconColor, string Label)
{
	b32 Clicked = false;

	quad Target = {0};
	Target.X = MenuContext.OffsetX - ICON_SIZE/2;
	Target.Y = MenuContext.OffsetY;
	Target.Width = ICON_SIZE;
	Target.Height = ICON_SIZE;
	Target.Color = IconColor;
	Target.TextureID = Icon;

	MenuContext.Width = Max(MenuContext.Width, Target.Width);
	MenuContext.Height += Target.Height + PADDING;
	MenuContext.OffsetY += Target.Height + PADDING;

	// NOTE(ariel) Hot means mouse over menu button. Active means holding mouse
	// button over menu button. Clicked means user releases mouse button in this
	// frame after pressing it previously.
	if (MouseOverTarget(Target))
	{
		// TODO(ariel) Display label if hot.
		Target.Color -= 0x11000000;
		MenuContext.HotID = ID;
		if (MenuContext.MouseDown)
		{
			Target.Color -= 0x22000000;
			MenuContext.ActiveID = ID;
		}
		else if (MenuContext.PreviousMouseDown)
		{
			Clicked = true;
		}
	}

	MenuContext.Commands[MenuContext.CommandCount] = Target;
	MenuContext.CommandCount += 1;

	return Clicked;
}
