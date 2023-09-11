enum
{
	ICON_SIZE = 24,
	PADDING = 8,
};

static inline b32
MouseOverTarget(quad Target)
{
	b32 X0 = Input.MousePositionX >= Target.X;
	b32 X1 = Input.MousePositionX <= Target.X + Target.Width;
	b32 Y0 = Input.MousePositionY >= Target.Y;
	b32 Y1 = Input.MousePositionY <= Target.Y + Target.Height;
	b32 Result = X0 & X1 & Y0 & Y1;
	return Result;
}

static void
BeginMenu(void)
{
	MenuContext.MenuIsHot = MouseOverTarget(MenuContext.EntireMenu);
	if (MenuContext.MenuIsHot)
	{
		PlatformShowCursor();
	}
	else
	{
		PlatformHideCursor();
	}

	MenuContext.OffsetX = MenuContext.StartOffsetX = WINDOW_WIDTH - ICON_SIZE*MenuContext.MenuIsHot;
	MenuContext.OffsetY = MenuContext.StartOffsetY = WINDOW_HEIGHT/2 - (ICON_SIZE+PADDING)/2 - MenuContext.Height/2;
	MenuContext.Width = 0;
	MenuContext.Height = 0;
	MenuContext.HotID = 0;
	MenuContext.CommandCount = 1;
}

static void
FinishMenu(void)
{
	// NOTE(ariel) Don't draw until after first frame because menu isn't centered
	// properly yet.
	if (MenuContext.StartOffsetY == WINDOW_HEIGHT/2 - ICON_SIZE/2)
	{
		MenuContext.CommandCount = 0;
	}

	MenuContext.PreviousFrameMouseDown = Input.MouseDown;

	MenuContext.EntireMenu.X -= ICON_SIZE;
	MenuContext.EntireMenu.Y -= ICON_SIZE / 2;
	MenuContext.EntireMenu.Width += ICON_SIZE;
	MenuContext.EntireMenu.Height += ICON_SIZE;
	MenuContext.EntireMenu.Color = 0xffffffff;
	MenuContext.EntireMenu.TextureID = MENU_ICON_BLANK;
	MenuContext.Commands[0] = MenuContext.EntireMenu;
}

static b32
MenuButtonID(u32 ID, menu_icon Icon, u32 IconColor)
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
		MenuContext.HotID = ID;

		if (!MenuContext.PreviousFrameMouseDown && Input.MouseDown)
		{
			MenuContext.ActiveID = ID;
		}
		else if (MenuContext.ActiveID == ID && MenuContext.PreviousFrameMouseDown && !Input.MouseDown)
		{
			Clicked = true;
			MenuContext.ActiveID = 0;
		}

		Target.Color -= 0x11000000*(MenuContext.HotID == ID) + 0x11000000*(MenuContext.ActiveID == ID);
	}

	MenuContext.Commands[MenuContext.CommandCount] = Target;
	MenuContext.CommandCount += 1;

	return Clicked;
}
