enum { ICON_SIZE = 32 };

static void
BeginMenu(void)
{
	// TODO(ariel) Draw white background for menu?
	MenuContext.OffsetX = MenuContext.StartOffsetX = WINDOW_WIDTH - ICON_SIZE;
	MenuContext.OffsetY = MenuContext.StartOffsetY = WINDOW_HEIGHT/2 - ICON_SIZE/2;
	MenuContext.Width = 0;
	MenuContext.Height = 0;
	MenuContext.HotID = 0;
	MenuContext.CommandCount = 0;
}

static void
EndMenu(void)
{
	MenuContext.PreviousMouseDown = MenuContext.MouseDown;
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
MenuButtonID(u32 ID, u32 IconColor, string Label)
{
	b32 Clicked = false;

	quad Target = {0};
	Target.X = MenuContext.OffsetX;
	Target.Y = MenuContext.OffsetY;
	Target.Width = ICON_SIZE;
	Target.Height = ICON_SIZE;
	Target.Color = IconColor;

	MenuContext.Width = Max(MenuContext.Width, Target.Width);
	MenuContext.Height += ICON_SIZE;
	MenuContext.OffsetY += ICON_SIZE;

	// NOTE(ariel) Hot means mouse over menu button. Active means holding mouse
	// button over menu button. Clicked means user releases mouse button in this
	// frame after pressing it previously.
	if (MouseOverTarget(Target))
	{
		// TODO(ariel) Display label if hot.
		Target.Color += 0x11111100;
		MenuContext.HotID = ID;
		if (MenuContext.MouseDown)
		{
			Target.Color += 0x22222200;
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
