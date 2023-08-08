#ifdef DEBUG
static void
OpenGLDebugMessageCallback(
	GLenum Source,
	GLenum Type,
	GLuint Id,
	GLenum Severity,
	GLsizei Length,
	const GLchar *Message,
	const void *UserParam)
{
	(void)Source;
	(void)Id;
	(void)Severity;
	(void)Length;
	(void)UserParam;
	if (Type == GL_DEBUG_TYPE_ERROR)
	{
		fprintf(stderr, "GL ERROR: %s\n", Message);
	}
	else
	{
		fprintf(stderr, "GL INFO: %s\n", Message);
	}
}
#endif

// TODO(ariel) Reduce this. I only need to project X and Y.
static void
SetOrthographicProjection(GLuint ShaderProgram)
{
	// NOTE(ariel) Flip `Bottom` and `Top` since to map Y onto (relatively)
	// inverted OpenGL coordinate system.
	f32 Left = 0.0f;
	f32 Right = WINDOW_WIDTH;
	f32 Bottom = WINDOW_HEIGHT;
	f32 Top = 0.0f;
	f32 Near = -1.0f;
	f32 Far = 1.0f;

	f32 X  = +2.0f / (Right - Left);
	f32 Y  = +2.0f / (Top - Bottom);
	f32 Z  = -2.0f / (Far - Near);
	f32 TX = -(Right + Left) / (Right - Left);
	f32 TY = -(Top + Bottom) / (Top - Bottom);
	f32 TZ = -(Far + Near) / (Far - Near);

	f32 OrthographicProjection[4][4] =
	{
		{    X,  0.0f,  0.0f, 0.0f, },
		{  0.f,     Y,  0.0f, 0.0f, },
		{  0.f,  0.0f,     Z, 0.0f, },
		{   TX,    TY,    TZ, 1.0f, },
	};

	GLint UniformLocation = glGetUniformLocation(ShaderProgram, "OrthographicProjection");
	glUniformMatrix4fv(UniformLocation, 1, GL_FALSE, (f32 *)OrthographicProjection);
}

// FIXME(ariel) Free any resources allocated. I don't believe GPU memory works
// like CPU memory, namely shader program and stuff like that.
static void
InitializeRenderer(void)
{
	LoadOpenGLExtensions();

	// NOTE(ariel) Set global state of OpenGL context.
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

#ifdef DEBUG
	{
		s32 OpenGLMajorVersion = 0;
		s32 OpenGLMinorVersion = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &OpenGLMajorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &OpenGLMinorVersion);
		fprintf(stderr, "OpenGL %d.%d: %s %s\n",
			OpenGLMajorVersion, OpenGLMinorVersion,
			glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	}

	// NOTE(ariel) Output any and all debug messages from OpenGL.
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLDebugMessageCallback, 0);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
	}
#endif

	// NOTE(ariel) Build and link shader program.
	{
		const char *VertexShaderSource =
			"#version 330 core\n"
			"uniform mat4 OrthographicProjection;\n"
			"layout (location = 0) in vec2 Position;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = OrthographicProjection * vec4(Position, 0.0f, 1.0f);\n"
			"}\n";
		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexShaderSource, 0);
		glCompileShader(VertexShader);

		const char *FragmentShaderSource =
			"#version 330 core\n"
			"out vec4 Color;\n"
			"void main()\n"
			"{\n"
			"	Color = vec4(1.0f);\n"
			"}\n";
		GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(FragmentShader, 1, &FragmentShaderSource, 0);
		glCompileShader(FragmentShader);

		GLint CompileStatus = 0;
		glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &CompileStatus); Assert(CompileStatus);
		glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &CompileStatus); Assert(CompileStatus);

		GLuint ShaderProgram = glCreateProgram();
		glAttachShader(ShaderProgram, VertexShader);
		glAttachShader(ShaderProgram, FragmentShader);
		glLinkProgram(ShaderProgram);

		GLint LinkStatus = 0;
		glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &LinkStatus); Assert(LinkStatus);

		glUseProgram(ShaderProgram);
		SetOrthographicProjection(ShaderProgram);

		glDeleteShader(FragmentShader);
		glDeleteShader(VertexShader);
	}

	// NOTE(ariel) Allocate and load arrays on GPU.
	{
		GLuint VertexArray = 0;
		glGenVertexArrays(1, &VertexArray);
		glBindVertexArray(VertexArray);

		GLuint VerticesBuffer = 0;
		glGenBuffers(1, &VerticesBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, VerticesBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STREAM_DRAW);

		glVertexAttribPointer(0, 2, GL_INT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}

	Assert(glGetError() == GL_NO_ERROR);
}

static void
ClearBuffer(void)
{
	for (s32 Y = 0; Y < WINDOW_HEIGHT; Y += 1)
	{
		for (s32 X = 0; X < WINDOW_WIDTH; X += 1)
		{
			Framebuffer[WINDOW_WIDTH*Y + X] = 0xffff00ff;
		}
	}
}

static void
PresentBuffer(void)
{
	glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesCount * sizeof(Vertices[0]), Vertices);
	glDrawArrays(GL_POINTS, 0, VerticesCount);
	glXSwapBuffers(X11Display, X11Window);
}

static inline u64
GetTime(void)
{
	u64 Nanoseconds = 0;
	struct timespec Now = {0};
	clock_gettime(CLOCK_MONOTONIC, &Now);
	Nanoseconds += Now.tv_sec;
	Nanoseconds *= NANOSECONDS_PER_SECOND;
	Nanoseconds += Now.tv_nsec;
	return Nanoseconds;
}

static inline void
SleepForNanoseconds(u64 DeltaTimeNS)
{
	static const u64 TARGET_FRAME_TIME_NS = NANOSECONDS_PER_SECOND / TARGET_FRAMES_PER_SECOND;
	s64 SleepTimeNS = TARGET_FRAME_TIME_NS - DeltaTimeNS;
	if (SleepTimeNS > 0)
	{
		struct timespec Time = {0};
		Time.tv_sec = SleepTimeNS / NANOSECONDS_PER_SECOND;
		Time.tv_nsec = SleepTimeNS % NANOSECONDS_PER_SECOND;
		nanosleep(&Time, 0);
	}
}
