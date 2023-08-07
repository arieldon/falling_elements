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

static void
InitializeDummyTexture(void)
{
	GLuint Texture = 0;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &Texture);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, Framebuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
			"layout (location = 0) in vec2 Position;\n"
			"out vec2 TextureCoordinate;\n"
			"void main()\n"
			"{\n"
			"	TextureCoordinate = Position * 0.5 + 0.5;\n"
			"	gl_Position = vec4(Position, 0.0, 1.0);\n"
			"}\n";
		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexShaderSource, 0);
		glCompileShader(VertexShader);

		const char *FragmentShaderSource =
			"#version 330 core\n"
			"uniform sampler2D Texture;\n"
			"in vec2 TextureCoordinate;\n"
			"out vec4 Color;\n"
			"void main()\n"
			"{\n"
			"	Color = texture(Texture, TextureCoordinate);\n"
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
	}

	// NOTE(ariel) Allocate and load arrays on GPU.
	{
		GLuint VertexArray = 0;
		glGenVertexArrays(1, &VertexArray);
		glBindVertexArray(VertexArray);

		GLfloat DummyVertices[] =
		{
			// NOTE(ariel) Encode poitns of top-right triangle.
			-1.0f, +1.0f,
			+1.0f, +1.0f,
			+1.0f, -1.0f,

			// NOTE(ariel) Encode points of bottom-left triangle.
			+1.0f, -1.0f,
			-1.0f, -1.0f,
			-1.0f, +1.0f,
		};
		GLuint VerticesBuffer = 0;
		glGenBuffers(1, &VerticesBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, VerticesBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DummyVertices), DummyVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		InitializeDummyTexture();
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
	s32 XStart = 0;
	s32 YStart = 0;
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		XStart, YStart, WINDOW_WIDTH, WINDOW_HEIGHT,
		GL_RGBA, GL_UNSIGNED_BYTE, Framebuffer);
	glDrawArrays(GL_TRIANGLES, 0, 6);
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
